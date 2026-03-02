#include "foxglove/vulkan/swapchain.h"

#include "VkBootstrap.h"

Swapchain::Swapchain() {}
Swapchain::~Swapchain() {}

#include <iostream>
void Swapchain::init(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    // initialize the swapchain
    m_device = device;

    m_physical_device = physical_device;
    m_surface = surface;

    vkb::SwapchainBuilder swapchain_builder { m_physical_device, m_device, m_surface };
    m_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
        .set_desired_format(VkSurfaceFormatKHR{
                .format = m_image_format,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    m_extent = vkb_swapchain.extent;
    m_swapchain = vkb_swapchain.swapchain;
    m_images = vkb_swapchain.get_images().value();
    m_image_views = vkb_swapchain.get_image_views().value();
}

void Swapchain::cleanup() {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    for(size_t i = 0; i < m_image_views.size(); ++i) {
        vkDestroyImageView(m_device, m_image_views[i], nullptr);
    }
}
