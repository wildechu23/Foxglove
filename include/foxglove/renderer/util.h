#pragma once

#include <vulkan/vulkan.h>

#include "foxglove/resources/fg_resource_types.h"

namespace util {
    /*
	void transition_image(
		VkCommandBuffer cmd,
		VkImage image,
		VkImageLayout currentLayout,
		VkImageLayout newLayout
	);*/

    void transition_image_to_present(VkCommandBuffer cmd, VkImage image);

	void copy_image_to_image(
		VkCommandBuffer cmd,
		VkImage source,
		VkImage destination,
		VkExtent2D srcSize,
		VkExtent2D dstSize
	);

    //TODO: PROBABLY MOVE THIS SOMEWHERE ELSE
    // consider passcontext
    VkAccessFlags2 deduce_access_flags(BufferUsage usage);
    VkAccessFlags2 deduce_access_flags(TextureUsage usage);

    VkImageLayout deduce_layout(TextureUsage usage);
    VkPipelineStageFlags2 deduce_pipeline_flags(BufferUsage usage);
    VkPipelineStageFlags2 deduce_pipeline_flags(TextureUsage usage);

    VkDescriptorType deduce_descriptor_type(BufferUsage usage);
    VkDescriptorType deduce_descriptor_type(TextureUsage usage);

}
