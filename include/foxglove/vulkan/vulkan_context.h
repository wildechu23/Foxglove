#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

class Window;

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    void init(Window& window);
    void cleanup();

    VkInstance get_instance() const { return m_instance; }
    VkDevice get_device() const { return m_device; }
    VkPhysicalDevice get_physical_device() const { return m_physical_device; }
    VkSurfaceKHR get_surface() const { return m_surface; }
    VmaAllocator get_allocator() const { return m_allocator; }

    VkQueue get_graphics_queue() { return m_graphics_queue; }
    uint32_t get_graphics_queue_family() const { return m_graphics_queue_family_index; }
    
    VkQueue get_transfer_queue() { return m_transfer_queue; }
    uint32_t get_transfer_queue_family() const { return m_transfer_queue_family_index; }
private:
    bool bUseValidationLayers = true;
    
    VkInstance m_instance;
    VkDevice m_device;
    VkPhysicalDevice m_physical_device;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkSurfaceKHR m_surface;

    VkQueue m_graphics_queue;
    uint32_t m_graphics_queue_family_index;

    VkQueue m_transfer_queue;
    uint32_t m_transfer_queue_family_index;

    VmaAllocator m_allocator;
};
