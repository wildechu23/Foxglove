#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class GPUImage {
public:
    GPUImage();
    ~GPUImage();

    void init(VkDevice device, VmaAllocator allocator, 
            VkFormat format, VkExtent2D extent,
            VkImageUsageFlags usage,
            VkImageAspectFlags aspect);
    void create(VkDevice device, VmaAllocator allocator,
        VkFormat format, VkExtent3D ext, VkImageUsageFlags usage);
    void cleanup(VkDevice device, VmaAllocator);

    VkImage& get_image() { return m_image; }
    VmaAllocation& get_allocation() { return m_allocation; }
    VkImageView& get_image_view() { return m_image_view; }
    VkFormat get_format() const { return m_format; }
    VkExtent2D get_extent() const { return m_extent; }

    void set_format(VkFormat format) { m_format = format; }

private:
    VkImage m_image;
    VmaAllocation m_allocation;
    VkImageView m_image_view;
    VkFormat m_format;
    VkExtent2D m_extent;
};
