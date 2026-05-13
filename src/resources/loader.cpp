#include "foxglove/resources/loader.h"
#include "foxglove/resources/resource_manager.h"
#include "foxglove/resources/upload_manager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <fastgltf/glm_element_traits.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

VkFilter extract_filter(fastgltf::Filter filter) {
    switch (filter) {
        // nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

            // linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {
    switch (filter) {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::optional<std::shared_ptr<LoadedGLTF>> Loader::load_gltf_meshes(
        fs::path file_path) {
    std::cout << "Loading GLTF: " << file_path << std::endl;

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    LoadedGLTF& file = *scene.get();

    auto data = fastgltf::GltfDataBuffer::FromPath(file_path);
    if (data.error() != fastgltf::Error::None) {
        std::cerr << "Failed to load glTF file: " 
            << fastgltf::getErrorMessage(data.error()) << std::endl;
    }

    constexpr auto gltf_options = fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser {};


    auto type = fastgltf::determineGltfFileType(data.get());
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(data.get(), file_path.parent_path(), 
                gltf_options);
        if (!load) {
            std::cerr << "Failed to load glTF: " 
                << fastgltf::to_underlying(load.error()) << std::endl;
            return std::nullopt;
        }

        gltf = std::move(load.get());
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(data.get(), file_path.parent_path(),
                gltf_options);
        if (!load) {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return std::nullopt;
        }

        gltf = std::move(load.get());
    } else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return std::nullopt;
    }

    std::vector<UploadJobHandle> jobs;
    
    // temporary storage
    std::vector<TextureHandle> images;
    std::vector<std::shared_ptr<MeshData>> meshes;

    // load samplers
    fastgltf::Filter default_filter = fastgltf::Filter::Nearest;
    for (fastgltf::Sampler& sampler : gltf.samplers) {
        SamplerHandle sampler_h = m_rm.create_sampler(SamplerDesc{
            .mag_filter = extract_filter(sampler.magFilter.value_or(
                        default_filter)),
            .min_filter = extract_filter(sampler.minFilter.value_or(
                        default_filter)),
            .mipmap_mode = extract_mipmap_mode(sampler.minFilter.value_or(
                        default_filter)),
            .min_lod = 0,
            .max_lod = VK_LOD_CLAMP_NONE
        });

        file.samplers.push_back(sampler_h);
    }

    for (fastgltf::Image& image : gltf.images) {
        std::optional<TextureHandle> img = load_image(gltf, image);

        if (img.has_value()) {
            images.push_back(*img);
            file.images[std::string(image.name)] = *img;
        }
        else {
            //images.push_back(engine->_errorCheckerboardImage);
            std::cout << "gltf failed to load texture " 
                << image.name << std::endl;
		}
	}

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        std::shared_ptr<MeshData> new_mesh = std::make_shared<MeshData>();
        meshes.push_back(new_mesh);
        
        file.meshes[std::string(mesh.name)] = new_mesh;
        new_mesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for(auto&& p : mesh.primitives) {
            GeoSurface new_surface;
            new_surface.start_index = static_cast<uint32_t>(indices.size());
            new_surface.count = static_cast<uint32_t>(
                    gltf.accessors[p.indicesAccessor.value()].count
            );

            size_t initial_vertex = vertices.size();

            // load indices
            {
                size_t index = p.indicesAccessor.value();
                fastgltf::Accessor& index_accessor = gltf.accessors[index];
                indices.reserve(indices.size() + index_accessor.count);
         
                fastgltf::iterateAccessor<uint32_t>(gltf, index_accessor,
                    [&](uint32_t idx) {
                        indices.push_back(idx + initial_vertex);
                    });
            }
       
            // load vertex positions
            {
                size_t index = p.findAttribute("POSITION")->accessorIndex;
                fastgltf::Accessor& pos_accessor = gltf.accessors[index];
                vertices.resize(vertices.size() + pos_accessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, pos_accessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex new_vtx;
                        new_vtx.position = v;
                        new_vtx.normal = { 1, 0, 0 };
                        new_vtx.color = glm::vec4 { 1.f };
                        new_vtx.uv_x = 0.f;
                        new_vtx.uv_y = 0.f;
                        vertices[initial_vertex + index] = new_vtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, 
                        gltf.accessors[normals->accessorIndex],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vertex + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, 
                        gltf.accessors[uv->accessorIndex],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vertex + index].uv_x = v.x;
                        vertices[initial_vertex + index].uv_y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, 
                        gltf.accessors[colors->accessorIndex],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vertex + index].color = v;
                    });
            }
            new_mesh->surfaces.push_back(new_surface);

            // display the vertex normals
            constexpr bool override_colors = true;
            if (override_colors) {
                for (Vertex& vtx : vertices) {
                    vtx.color = glm::vec4(vtx.normal, 1.f);
                }
            }
            
            /*
            for(Vertex& vtx : vertices) {
                std::cout << vtx.position.x <<
                    vtx.position.y <<
                    vtx.position.z << std::endl;
            }
            */

            const size_t vbuffer_size = vertices.size() * sizeof(Vertex);
            const size_t ibuffer_size = indices.size() * sizeof(uint32_t);
            
            new_mesh->vertex_buffer = m_rm.create_buffer({
                vbuffer_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            });

            new_mesh->index_buffer = m_rm.create_buffer({
                ibuffer_size, 
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            });

            // TODO: BUFFER HANDLE?
            jobs.push_back(m_um.upload_data(vertices.data(), 
                        vbuffer_size, new_mesh->vertex_buffer));
            jobs.push_back(m_um.upload_data(indices.data(),
                        ibuffer_size, new_mesh->index_buffer));
        }
    }

    // TODO: COMBINE TEXTURE JOBS INTO BATCH?
    m_um.submit_batch();

    
    for(UploadJobHandle job : jobs) {
        m_um.wait_for_handle(job);
    }
    std::cout << "loader jobs done" << std::endl;
    m_um.process_completions();

    return scene;
}



std::optional<TextureHandle> Loader::load_image(fastgltf::Asset& asset, 
        fastgltf::Image& image) {
    TextureHandle new_image;
    int width, height, nrChannels;

    std::visit(fastgltf::visitor { [](auto& arg) {},
        [&](fastgltf::sources::URI& file_path) {
            assert(file_path.fileByteOffset == 0); // offsets not supported
            assert(file_path.uri.isLocalPath()); // local files only 
            
            const std::string path(file_path.uri.path().begin(),
                    file_path.uri.path().end()); 
            
            unsigned char* data = stbi_load(path.c_str(), &width, &height, 
                    &nrChannels, 4);
            if (!data) return;

            const uint32_t width_u = static_cast<uint32_t>(width);
            const uint32_t height_u = static_cast<uint32_t>(height);

            VkExtent3D image_extent {
                width_u,
                height_u,
                1
            };

            new_image = m_rm.create_texture(TextureDesc{
                .extent = VkExtent2D{width_u, height_u},
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT
            });

            UploadJobHandle j1 = m_um.upload_data(data, image_extent, 
                    new_image);

            m_um.submit_batch();
            m_um.wait_for_handle(j1);
            m_um.process_completions();

            stbi_image_free(data);   
        },
        [&](fastgltf::sources::Vector& vector) {
            unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(vector.bytes.data()), 
                    static_cast<int>(vector.bytes.size()),
                    &width, &height, &nrChannels, 4);
            if (!data) return;

            const uint32_t width_u = static_cast<uint32_t>(width);
            const uint32_t height_u = static_cast<uint32_t>(height);

            VkExtent3D image_extent {
                width_u,
                height_u,
                1
            };

            new_image = m_rm.create_texture(TextureDesc{
                .extent = VkExtent2D{width_u, height_u},
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT
            });

            UploadJobHandle j1 = m_um.upload_data(data, image_extent, 
                    new_image);
            
            m_um.submit_batch();
            m_um.wait_for_handle(j1);
            m_um.process_completions();
            

            stbi_image_free(data);
        },
        [&](fastgltf::sources::BufferView& view) {
            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            // We only care about VectorWithMime because LoadExternalBuffers
            // buffers are already loaded into a vector.
            std::visit(fastgltf::visitor { [](auto& arg) {},
                [&](fastgltf::sources::Vector& vector) {
                    unsigned char* data = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc*>(vector.bytes.data() 
                                + bufferView.byteOffset),
                            static_cast<int>(bufferView.byteLength),
                            &width, &height, &nrChannels, 4);

                    if(!data) return;

                    const uint32_t width_u = static_cast<uint32_t>(width);
                    const uint32_t height_u = static_cast<uint32_t>(height);
                    
                    VkExtent3D image_extent {
                        width_u,
                        height_u,
                        1
                    };

                    new_image = m_rm.create_texture(TextureDesc{
                        .extent = VkExtent2D{width_u, height_u},
                        .format = VK_FORMAT_R8G8B8A8_UNORM,
                        .usage = VK_IMAGE_USAGE_SAMPLED_BIT
                    });

                    UploadJobHandle j1 = m_um.upload_data(data, image_extent, 
                            new_image);

                    m_um.submit_batch();
                    m_um.wait_for_handle(j1);
                    m_um.process_completions();


                    stbi_image_free(data);
                }}, buffer.data);
        },
    }, image.data);

    if(m_rm.get_texture(new_image)->image == VK_NULL_HANDLE) {
        return std::nullopt;
    }

    return new_image;
}
