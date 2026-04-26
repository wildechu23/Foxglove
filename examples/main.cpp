#include "foxglove/window/window.h"
#include "foxglove/renderer/renderer.h"
#include "foxglove/framegraph/framegraph.h"
#include "foxglove/resources/loader.h"
#include "foxglove/resources/upload_manager.h"
#include "foxglove/core/camera.h"
#include "foxglove/core/input_manager.h"


namespace fs = std::filesystem;

int main() {
    uint32_t width = 1280;
    uint32_t height = 720;
    Window window(width, height, "Test Engine");
    window.init();

    InputManager input_manager(window);
    Camera camera;
    
    fs::path src_dir = std::getenv("TEST_SOURCE_DIR");

    Renderer renderer(window);
    FrameGraph& fg = renderer.get_fg();
    ShaderLibrary& sl = renderer.get_sl();
    PipelineManager& pm = renderer.get_pm();
    ResourceManager& rm = renderer.get_rm();
    UploadManager& um = renderer.get_um();

    // load meshes
    Loader loader(rm, um);
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
    gcb.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    gcb.set_multisampling_none();
    gcb.disable_blending();
    gcb.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    gcb.set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT);
    gcb.set_depth_format(VK_FORMAT_D32_SFLOAT);

    GraphicsPipelineConfig config = gcb.build();
    GraphicsPipeline* triangle = pm.get_graphics_pipeline(config);

    VkImageUsageFlags draw_image_usages = VkImageUsageFlags(
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    using clock = std::chrono::high_resolution_clock;
    constexpr double max_update_dt = 0.1;
    auto previous_time = clock::now();
    
    while(!window.should_close()) {
        auto current_time = clock::now();
        std::chrono::duration<double> elapsed = current_time - previous_time;
        previous_time = current_time;

        double dt = elapsed.count();
        if (dt > max_update_dt) dt = max_update_dt;

        window.update();
        input_manager.update();
        camera.update(input_manager, dt);

        um.process_completions();

        struct PushConstant {
            glm::mat4 world_matrix;
            VkDeviceAddress vertex_buffer;
        };
        
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 projection = camera.get_projection_matrix();

        PushConstant pc = {
            .world_matrix = projection * view,
            .vertex_buffer = rm.get_buffer_address(mesh0.vertex_buffer)
        };

        FGBufferHandle mesh_i = fg.register_external_buffer(
                "mesh index buffer", rm.get_buffer(mesh0.index_buffer));

        FGTextureHandle draw_image = fg.create_texture("draw image", 
            TextureDesc{
                .extent = {width, height},
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = draw_image_usages,
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

        um.submit_batch();

        renderer.draw();
    }


    return 0;
}
