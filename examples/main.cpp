#include "foxglove/core/engine.h"
#include "foxglove/renderer/renderer.h"
#include "foxglove/resources/loader.h"

namespace fs = std::filesystem;

int main() {
    uint32_t width = 1280;
    uint32_t height = 720;
    fs::path src_dir = std::getenv("TEST_SOURCE_DIR");
    
    EngineConfig eng_config;
    eng_config.window = { width, height, "Test Engine" };
    eng_config.enable_input = true;
    eng_config.camera = { .fov = 80.f, .aspect_ratio = (float)width / height };
    eng_config.graphics = { .enable_resources = true, .enable_upload = true };

    Engine engine(eng_config);

    ResourceManager* rm = engine.resources();
    UploadManager* um = engine.upload_manager();
    Renderer* renderer = engine.renderer();

    ShaderLibrary& sl = renderer->get_sl();
    PipelineManager& pm = renderer->get_pm();
    
    // load meshes
    Loader loader(*rm, *um);
    std::shared_ptr<LoadedGLTF> gltf = 
        loader.load_gltf_meshes(src_dir/"assets/Avocado.glb").value();
    
    std::cout << "meshes:" << gltf->meshes.size() << std::endl;
    for(auto [key, mesh] : gltf->meshes) {
        std::cout << " - " << key << std::endl;
    }
    std::cout << "images: " << gltf->images.size() <<  std::endl;
    for(auto [key, image] : gltf->images) {
        std::cout << " - " << key << std::endl;
    }
    std::cout << "samplers: " << gltf->samplers.size() << std::endl;

    MeshData& mesh0 = *(gltf->meshes["Avocado"]);
    TextureHandle texture_r = gltf->images["0"];
    

    ComputeShader* gradient_shader = sl.create_compute_shader(
            fs::path("shaders/gradient_heap.comp.spv"));
    ComputeShader* blur_shader = sl.create_compute_shader(
            fs::path("shaders/radial_blur.comp.spv"));
    ComputeShader* draw_image_shader = sl.create_compute_shader(
            fs::path("shaders/draw_image.comp.spv"));


    VertexShader* vert = sl.create_vertex_shader(
            fs::path("shaders/colored_triangle_mesh.vert.spv"));
    FragmentShader* frag = sl.create_fragment_shader(
            fs::path("shaders/tex_image.frag.spv"));

    ComputePipeline* background = pm.get_compute_pipeline(gradient_shader);
    ComputePipeline* blur = pm.get_compute_pipeline(blur_shader);
    ComputePipeline* draw = pm.get_compute_pipeline(draw_image_shader);
    
    // TODO: simplify gcb and change draw_image_usages
    GraphicsConfigBuilder gcb;
    gcb.set_shaders({vert, frag, nullptr});
    gcb.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gcb.set_polygon_mode(VK_POLYGON_MODE_FILL);
    gcb.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    gcb.set_multisampling_none();
    gcb.disable_blending();
    gcb.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    gcb.set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT);
    gcb.set_depth_format(VK_FORMAT_D32_SFLOAT);

    GraphicsPipelineConfig config = gcb.build();
    GraphicsPipeline* triangle = pm.get_graphics_pipeline(config);

    // TODO: CREATE RESIZING DRAW IMAGES
    TextureHandle draw_image_r = rm->create_texture(TextureDesc{
        .extent = {width, height},
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_STORAGE_BIT
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    });

    TextureHandle depth_image_r = rm->create_texture(TextureDesc{
        .extent = {width, height},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    });

    SamplerHandle sampler_r = rm->create_sampler(SamplerDesc{
        .mag_filter = VK_FILTER_NEAREST,
        .min_filter = VK_FILTER_NEAREST,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .min_lod = 0,
        .max_lod = VK_LOD_CLAMP_NONE
    });

    FrameGraph& fg = renderer->get_fg();
    while(!engine.window()->should_close()) {
        engine.begin_frame();
        
        FGTextureHandle fake_image = fg.create_texture("fake image", 
            TextureDesc{
                .extent = {width, height},
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage =  VK_IMAGE_USAGE_TRANSFER_DST_BIT
                | VK_IMAGE_USAGE_STORAGE_BIT
                | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            });

        glm::mat4 view = engine.camera()->get_view_matrix();
        glm::mat4 projection = engine.camera()->get_projection_matrix();

        FGBufferHandle mesh_i = fg.import_buffer(
                "mesh index buffer", mesh0.index_buffer);
        FGTextureHandle draw_image = fg.import_texture(
                "draw image", draw_image_r);
        FGTextureHandle depth_image = fg.import_texture(
                "depth image", depth_image_r);

        FGTextureHandle texture = fg.import_texture("texture", texture_r);
        FGSamplerHandle sampler = fg.import_sampler("sampler", sampler_r);
        
        fg.create_pass("test", PassType::Clear)
            .clear_color(draw_image, Color{0.7f, 0.5f, 0.7f, 1.f})
            .build();
        /* 
        fg.create_pass("compute", PassType::Compute)
            .bind_texture(fake_image, TextureUsage::StorageImage,
                    ResourceAccess::ReadWrite, 0)
            .execute([&](PassContext ctx) {
                ctx.bind_compute_pipeline(background);
                ctx.dispatch_compute(std::ceil(1280/16.0),
                        std::ceil(720/16.0), 1);
            })
            .build();
        */

        
        struct DrawPushConstant {
            glm::vec2 src_offset;
            glm::vec2 dst_offset;
            glm::vec2 extent;
        };
        DrawPushConstant dpc = {
            .src_offset = {0, 0},
            .dst_offset = {0, 0},
            .extent = {0,0}
        };

        fg.create_pass("draw image", PassType::Compute)
            .bind_texture(texture, TextureUsage::StorageImage,
                    ResourceAccess::Read, 0)
            .bind_texture(draw_image, TextureUsage::StorageImage,
                    ResourceAccess::Write, 1)
            .execute([&](PassContext ctx) {
                ctx.bind_compute_pipeline(draw);
                ctx.push_constant(dpc, 8);
                ctx.dispatch_compute(40, 40, 1);
            })
            .build();
            
         
        struct PushConstant {
            glm::mat4 world_matrix;
            VkDeviceAddress vertex_buffer;
        };
        PushConstant pc = {
            .world_matrix = projection * view,
            .vertex_buffer = rm->get_buffer_address(mesh0.vertex_buffer)
        };
        fg.create_pass("triangle", PassType::Graphics)
            .bind_texture(texture, TextureUsage::SampledImage,
                    ResourceAccess::Read, 0)
            .bind_sampler(sampler, 1)
            .bind_color_attachment(draw_image, 
                    LoadOp::Load, StoreOp::Store)
            .bind_depth_attachment(depth_image,
                    LoadOp::Clear, StoreOp::Store, 0.f)
            .execute([&](PassContext ctx) {
                ctx.bind_graphics_pipeline(triangle)
                    .bind_index_buffer(mesh_i)
                    .push_constant(pc, 16)
                    .draw_indexed(
                        mesh0.surfaces[0].count, 1,
                        mesh0.surfaces[0].start_index, 0, 0
                    );
            })
            .build();
            /*
        struct BlurPushConstant {
            glm::vec2 center;
            float start;
            float strength;
            int samples;
        };
        BlurPushConstant bpc = {
            .center = { 0.5, 0.5 },
            .start = 1.f,
            .strength = 0.05f,
            .samples = 10
        };

        // Add post-processing shader
        fg.create_pass("blur", PassType::Compute)
            .bind_texture(fake_image, TextureUsage::StorageImage,
                    ResourceAccess::Read, 0)
            .bind_texture(draw_image, TextureUsage::StorageImage,
                    ResourceAccess::Write, 1)
            .execute([&](PassContext ctx) {
                ctx.bind_compute_pipeline(blur);
                ctx.push_constant(bpc, 8);
                ctx.dispatch_compute(std::ceil(1280/16.0),
                        std::ceil(720/16.0), 1);
            })
            .build();
            */
        
        fg.create_pass("present", PassType::Present)
            .present(draw_image)
            .build();

        engine.draw();
    }


    return 0;
}
