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

    fs::path path("shaders/gradient.comp.spv");

    ComputeShader* shader = sl.create_compute_shader(path);
    ComputePipeline* pipeline = pm.get_compute_pipeline(shader);


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
        

        fg.create_pass("test", PassType::Graphics)
            .clear_color(draw_image, Color{0.f, 1.f, 0.f, 1.f})
            .build();
        
        fg.create_pass("compute", PassType::Compute)
            .bind_texture(draw_image, TextureUsage::StorageImage,
                    ResourceAccess::ReadWrite, 0, 0)
            .execute([&](PassContext ctx) {
                //std::cout << "hi" << std::endl;
                ctx.bind_compute_pipeline(pipeline);
                ctx.dispatch_compute(16, 16, 1);
            })
            .build();

        fg.create_pass("present", PassType::Present)
            .present(draw_image)
            .build();

        renderer.draw();
    }


    return 0;
}
