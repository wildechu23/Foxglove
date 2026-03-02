#pragma once

#include <vulkan/vulkan.h>

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
}
