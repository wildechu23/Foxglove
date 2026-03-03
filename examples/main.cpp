#include "foxglove/window/window.h"
#include "foxglove/renderer/renderer.h"
#include "foxglove/renderer/framegraph.h"

int main() {
    int width = 1280;
    int height = 720;
    Window window(width, height, "Test Engine");
    window.init();

    Renderer renderer(window);
    FrameGraph& fg = renderer.get_fg();
    
    VkImageUsageFlags draw_image_usages = VkImageUsageFlags(
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    FGTextureHandle draw_image = fg.create_texture(TextureDesc{
        .extent = {width, height},
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .usage = draw_image_usages,
    });

    fg.create_pass("test", PassType::Graphics)
        .bind_texture(draw_image, TextureUsage::StorageImage,
                ResourceAccess::Write)
        .clear_color(draw_image, Color{1.f, 0.f, 0.f, 1.f})
        .build();

    fg.create_pass("present", PassType::Present)
        .bind_present_source(draw_image)
        .build();
    
    
    while(!window.should_close()) {
        window.update();
        renderer.draw();
    }


    return 0;
}
