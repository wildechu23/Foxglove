#include "foxglove/vulkan/vulkan_context.h"

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanContext::VulkanContext() {}
VulkanContext::~VulkanContext() {}

void VulkanContext::init(GLFWwindow* window) {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Foxglove")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

    vkb::Instance vkb_inst = inst_ret.value();
    
    m_instance = vkb_inst.instance;
    m_debug_messenger = vkb_inst.debug_messenger;
	glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
    
	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
        .dynamicRendering = true
    };

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true
    };

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_surface(m_surface)
		.select()
		.value();

	//create the final vulkan device
	vkb::DeviceBuilder device_builder { physical_device };

	vkb::Device vkb_device = device_builder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	m_device = vkb_device.device;
	m_physical_device = physical_device.physical_device;
	
	m_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
	m_graphics_queue_family_index = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocator_info = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = m_physical_device,
		.device = m_device,
		.instance = m_instance
	};

	vmaCreateAllocator(&allocator_info, &m_allocator);
}

void VulkanContext::cleanup() {
    vmaDestroyAllocator(m_allocator);
   
    // delete vma allocator
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);

    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
    vkDestroyInstance(m_instance, nullptr);
}
