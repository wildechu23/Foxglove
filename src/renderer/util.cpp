#include "foxglove/renderer/util.h"

/*
void util::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {   
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    // src = last write
    // dst = next read
    // ALL COMMANDS bit is SLOW and STALLS GPU PIPELINE
    VkImageMemoryBarrier2 imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS
        }
    };
	
    VkDependencyInfo depInfo {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}*/

// expects offscreen rendering and transfer to swapchain
void util::transition_image_to_present(VkCommandBuffer cmd, VkImage image) {
    // transfer_dst -> present_src
    VkImageMemoryBarrier2 imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS
        }
    };
    
    VkDependencyInfo depInfo {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void util::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {
    // IF IDENTICAL FORMATS AND SIZE, VKCMDCOPYIMAGE
    // CHANGING FORMATS/SIZE, VKCMDBLITIMAGE

	VkImageBlit2 blitRegion = { 
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, 
        .pNext = nullptr,
        .srcSubresource = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 0, 1
        },
        .srcOffsets = {
            { 0, 0, 0 },
            {
                static_cast<int32_t>(srcSize.width), 
                static_cast<int32_t>(srcSize.height), 
                1
            }
        },
        .dstSubresource = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 0, 1
        },
        .dstOffsets = {
            { 0, 0, 0 },
            {
                static_cast<int32_t>(dstSize.width),
                static_cast<int32_t>(dstSize.height),
                1
            }
        }
    };

	VkBlitImageInfo2 blitInfo = { 
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext = nullptr,
        .srcImage = source,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = destination,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &blitRegion,
        .filter = VK_FILTER_LINEAR
    };

	vkCmdBlitImage2(cmd, &blitInfo);
}

// DEDUCE FLAGS
VkAccessFlags2 util::deduce_access_flags(BufferUsage usage) {
    switch(usage) {
        // TODO: FIGURE OUT READ WRITE
        case BufferUsage::StorageBuffer:
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | 
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        case BufferUsage::UniformBuffer:
            return VK_ACCESS_2_UNIFORM_READ_BIT;
        case BufferUsage::VertexBuffer:
            return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        case BufferUsage::IndexBuffer:
            return VK_ACCESS_2_INDEX_READ_BIT;
        case BufferUsage::TransferSrc:
            return VK_ACCESS_2_TRANSFER_READ_BIT;
        case BufferUsage::TransferDst:
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        default:
            return VK_ACCESS_2_NONE;
    }
}

VkAccessFlags2 util::deduce_access_flags(TextureUsage usage) {
    switch(usage) {
        // FIGURE OUT READ WRITE
        case TextureUsage::ColorAttachment:
            return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
        case TextureUsage::DepthAttachment:
        case TextureUsage::StencilAttachment:
        case TextureUsage::DepthStencilAttachment:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case TextureUsage::InputAttachment:
            return VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;
        case TextureUsage::StorageImage:
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        case TextureUsage::TransferSrc:
            return VK_ACCESS_2_TRANSFER_READ_BIT;
        case TextureUsage::TransferDst:
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        default:
            return VK_ACCESS_2_NONE;
    }
}

VkImageLayout util::deduce_layout(TextureUsage usage) {
    switch(usage) {
        case TextureUsage::ColorAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
        case TextureUsage::DepthAttachment:
            // TODO: CONSIDER OPTIONS FOR OTHER LAYOUTS
            // IE. READ ONLY OPTIONS
            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case TextureUsage::StencilAttachment:
            return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        case TextureUsage::DepthStencilAttachment:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case TextureUsage::InputAttachment:
            // TODO: CONSIDER OTHER READ ONLY LAYOUTS
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case TextureUsage::StorageImage:
            // TODO: IS THIS OPTIMIZABLE?
            return VK_IMAGE_LAYOUT_GENERAL;
        case TextureUsage::TransferSrc:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case TextureUsage::TransferDst:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkPipelineStageFlags2 util::deduce_pipeline_flags(BufferUsage usage) {
    switch(usage) {
        case BufferUsage::StorageBuffer:
            return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        case BufferUsage::UniformBuffer:
            return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        case BufferUsage::VertexBuffer:
            return VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
        case BufferUsage::IndexBuffer:
            return VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
        case BufferUsage::TransferSrc:
        case BufferUsage::TransferDst:
            return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        default:
            return VK_PIPELINE_STAGE_2_NONE;
    }
}

VkPipelineStageFlags2 util::deduce_pipeline_flags(TextureUsage usage) {
    switch(usage) {
        case TextureUsage::ColorAttachment:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case TextureUsage::DepthAttachment:
        case TextureUsage::StencilAttachment:
        case TextureUsage::DepthStencilAttachment:
            return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        case TextureUsage::InputAttachment:
            return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        case TextureUsage::StorageImage:
            return VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        case TextureUsage::TransferSrc:
        case TextureUsage::TransferDst:
            return VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        default:
            return VK_PIPELINE_STAGE_2_NONE;
    }
}

VkDescriptorType util::deduce_descriptor_type(BufferUsage usage) {
    switch(usage) {
        case BufferUsage::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case BufferUsage::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default:
            // TODO: SHOULD ERROR
            return VK_DESCRIPTOR_TYPE_SAMPLER;
    }
}

VkDescriptorType util::deduce_descriptor_type(TextureUsage usage) {
   switch(usage) {
        case TextureUsage::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case TextureUsage::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        default:
            // TODO: SHOULD ERROR
            return VK_DESCRIPTOR_TYPE_SAMPLER; 
    }

}
