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
    std::vector<std::shared_ptr<MeshData>> mesh = 
        loader.load_gltf_meshes(src_dir/"assets/basicmesh.glb").value();
    MeshData& mesh0 = *mesh[2];

    ComputeShader* shader = sl.create_compute_shader(
            fs::path("shaders/gradient.comp.spv"));

    VertexShader* vert = sl.create_vertex_shader(
            fs::path("shaders/colored_triangle_mesh.vert.spv"));
    FragmentShader* frag = sl.create_fragment_shader(
            fs::path("shaders/colored_triangle.frag.spv"));

    ComputePipeline* background = pm.get_compute_pipeline(shader);
    
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
    
    FrameGraph& fg = renderer->get_fg();
    while(!engine.window()->should_close()) {
        engine.begin_frame();
        glm::mat4 view = engine.camera()->get_view_matrix();
        glm::mat4 projection = engine.camera()->get_projection_matrix();

        struct PushConstant {
            glm::mat4 world_matrix;
            VkDeviceAddress vertex_buffer;
        };
        PushConstant pc = {
            .world_matrix = projection * view,
            .vertex_buffer = rm->get_buffer_address(mesh0.vertex_buffer)
        };

        FGBufferHandle mesh_i = fg.register_external_buffer(
                "mesh index buffer", mesh0.index_buffer);

        FGTextureHandle draw_image = fg.create_texture("draw image", 
            TextureDesc{
                .extent = {width, height},
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_STORAGE_BIT
                    | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            });

        FGTextureHandle depth_image = fg.create_texture("depth image", 
            TextureDesc{
                .extent = {width, height},
                .format = VK_FORMAT_D32_SFLOAT,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            });

        fg.create_pass("test", PassType::Clear)
            .clear_color(draw_image, Color{0.5f, 0.5f, 0.5f, 1.f})
            .build();
        /*
        fg.create_pass("compute", PassType::Compute)
            .bind_texture(draw_image, TextureUsage::StorageImage,
                    ResourceAccess::ReadWrite, 0, 0)
            .execute([&](PassContext ctx) {
                ctx.bind_compute_pipeline(background);
                ctx.dispatch_compute(16, 16, 1);
            })
            .build();
            */

        fg.create_pass("triangle", PassType::Graphics)
            .bind_color_attachment(draw_image, 
                    LoadOp::Load, StoreOp::Store)
            .bind_depth_attachment(depth_image,
                    LoadOp::Clear, StoreOp::Store, 0.f)
            .execute([&](PassContext ctx) {
                ctx.bind_graphics_pipeline(triangle)
                    .bind_index_buffer(mesh_i)
                    .push_constant(pc)
                    .draw_indexed(
                        mesh0.surfaces[0].count, 1,
                        mesh0.surfaces[0].start_index, 0, 0
                    );
            })
            .build();

        fg.create_pass("present", PassType::Present)
            .present(draw_image)
            .build();

        engine.draw();
    }


    return 0;
}
