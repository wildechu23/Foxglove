#include "foxglove/resources/loader.h"
#include "foxglove/resources/resource_manager.h"
#include "foxglove/resources/upload_manager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <iostream>

std::optional<std::vector<std::shared_ptr<MeshData>>> Loader::load_gltf_meshes(
        fs::path file_path) {
    std::cout << "Loading GLTF: " << file_path << std::endl;

    auto data = fastgltf::GltfDataBuffer::FromPath(file_path);

    if (data.error() != fastgltf::Error::None) {
        std::cerr << "Failed to load glTF file: " 
            << fastgltf::getErrorMessage(data.error()) << std::endl;
    }

    constexpr auto gltf_options = fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser {};

    auto load = parser.loadGltfBinary(data.get(), file_path.parent_path(),
            gltf_options);
    if (load.error() != fastgltf::Error::None) {
        std::cerr << "Failed to parse glTF file: " 
            << fastgltf::getErrorMessage(load.error()) << std::endl;
    }

    gltf = std::move(load.get());

    std::vector<std::shared_ptr<MeshData>> meshes;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        MeshData new_mesh;
        new_mesh.name = mesh.name;

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
            new_mesh.surfaces.push_back(new_surface);

            // display the vertex normals
            constexpr bool override_colors = true;
            if (override_colors) {
                for (Vertex& vtx : vertices) {
                    vtx.color = glm::vec4(vtx.normal, 1.f);
                }
            }
            
            for(Vertex& vtx : vertices) {
                std::cout << vtx.position.x <<
                    vtx.position.y <<
                    vtx.position.z << std::endl;
            }

            const size_t vbuffer_size = vertices.size() * sizeof(Vertex);
            const size_t ibuffer_size = indices.size() * sizeof(uint32_t);
            
            new_mesh.vertex_buffer = m_rm.create_buffer({
                vbuffer_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            });

            new_mesh.index_buffer = m_rm.create_buffer({
                ibuffer_size, 
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            });

            // TODO: BUFFER HANDLE?
            UploadJobHandle j1 = m_um.upload_data(vertices.data(), vbuffer_size, 
                    new_mesh.vertex_buffer);
            UploadJobHandle j2 = m_um.upload_data(indices.data(), ibuffer_size, 
                    new_mesh.index_buffer);

            m_um.submit_batch();
            m_um.wait_for_handle(j1);
            m_um.wait_for_handle(j2);
            std::cout << "jobs done" << std::endl;
            m_um.process_completions();

            meshes.emplace_back(std::make_shared<MeshData>(
                        std::move(new_mesh)));
        }
    }
    return meshes;
}
