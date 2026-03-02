#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Swapchain {
public:
    Swapchain();
    ~Swapchain();

    void init(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
    void cleanup();

    VkSwapchainKHR& get_swapchain() { return m_swapchain; }

    VkImage& get_image(int i) { return m_images[i]; }
    size_t get_image_count() { return m_images.size(); }
    VkExtent2D get_extent() { return m_extent; }

private:
    VkDevice m_device;
    VkPhysicalDevice m_physical_device;
    VkSurfaceKHR m_surface;
    
    VkSwapchainKHR m_swapchain;
    VkFormat m_image_format;
    VkExtent2D m_extent;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
};
