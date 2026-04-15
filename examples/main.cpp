#include "foxglove/window/window.h"
#include "foxglove/renderer/renderer.h"
#include "foxglove/renderer/framegraph.h"

namespace fs = std::filesystem;

int main() {
    uint32_t width = 1280;
    uint32_t height = 720;
    Window window(width, height, "Test Engine");
    window.init();
    
    Renderer renderer(window);
    FrameGraph& fg = renderer.get_fg();
    ShaderLibrary& sl = renderer.get_sl();
    PipelineManager& pm = renderer.get_pm();

    ComputeShader* shader = sl.create_compute_shader(
            fs::path("shaders/gradient.comp.spv"));

    VertexShader* vert = sl.create_vertex_shader(
            fs::path("shaders/colored_triangle.vert.spv"));
    FragmentShader* frag = sl.create_fragment_shader(
            fs::path("shaders/colored_triangle.frag.spv"));

    ComputePipeline* background = pm.get_compute_pipeline(shader);
    
    GraphicsConfigBuilder gcb;
    gcb.set_shaders({vert, frag, nullptr});
    gcb.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gcb.set_polygon_mode(VK_POLYGON_MODE_FILL);
    gcb.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    gcb.set_multisampling_none();
    gcb.disable_blending();
    gcb.disable_depthtest();
    gcb.set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT);
    gcb.set_depth_format(VK_FORMAT_UNDEFINED);

    GraphicsPipelineConfig config = gcb.build();
    GraphicsPipeline* triangle = pm.get_graphics_pipeline(config);


    VkImageUsageFlags draw_image_usages = VkImageUsageFlags(
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    while(!window.should_close()) {
        window.update();

        FGTextureHandle draw_image = fg.create_texture("draw image", 
            TextureDesc{
                .extent = {width, height},
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = draw_image_usages,
            });
        

        fg.create_pass("test", PassType::Clear)
            .clear_color(draw_image, Color{0.f, 1.f, 0.f, 1.f})
            .build();
        
        fg.create_pass("compute", PassType::Compute)
            .bind_texture(draw_image, TextureUsage::StorageImage,
                    ResourceAccess::ReadWrite, 0, 0)
            .execute([&](PassContext ctx) {
                //std::cout << "hi" << std::endl;
                ctx.bind_compute_pipeline(background);
                ctx.dispatch_compute(16, 16, 1);
            })
            .build();

        fg.create_pass("triangle", PassType::Graphics)
            .bind_color_attachment(draw_image, 
                    LoadOp::Load, StoreOp::Store)
            .execute([&](PassContext ctx) {
                ctx.bind_graphics_pipeline(triangle)
                    .bind_viewport({0, 0, 1280, 720, 0.f, 1.f})
                    .bind_scissor({0, 0, 1280, 720})
                    .draw(3, 1, 0, 0);
            })
            .build();

        fg.create_pass("present", PassType::Present)
            .present(draw_image)
            .build();

        renderer.draw();
    }


    return 0;
}
