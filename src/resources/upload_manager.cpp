#include "foxglove/resources/upload_manager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>

#include <iostream>
#include <cstring>

UploadManager::UploadManager(VulkanContext& ctx, ResourceManager& rm)
    : m_rm(rm) {
    m_device = ctx.get_device();
    m_allocator = ctx.get_allocator();
    m_transfer_queue = ctx.get_transfer_queue();
    m_transfer_queue_family = ctx.get_transfer_queue_family();
}

void UploadManager::init()  {
    // init commands
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_transfer_queue_family
    };

    vkCreateCommandPool(
            m_device,
            &cmd_pool_info, 
            nullptr,
            &m_cmd_pool);


    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(
            m_device,
            &cmd_alloc_info,
            &m_cmd_buffer);

    VkSemaphoreTypeCreateInfo semaphore_type_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };

    
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphore_type_info
    };
    vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, 
            &m_timeline_semaphore);
}



UploadJobHandle UploadManager::upload_data(const void* data, size_t size,
        BufferHandle dst) {
    // TODO: GET THESE FROM POOLS
    // staging buffer
    BufferHandle staging_handle = m_rm.create_buffer({
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_ONLY,
        .allocation_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
    });

    // TODO: WRITE FUNCTION TO GET PTR
    BufferResource* staging = m_rm.get_buffer(staging_handle);
    memcpy(staging->mapped_data, data, size);

    UploadJobHandle handle(m_next_handle_id++, 0);
    
    m_pending_uploads[handle] = PendingUpload{
        staging_handle,
        BufferUploadInfo {
            .dst = dst,
            .size = size
        }
    };

    return handle;
}


UploadJobHandle UploadManager::upload_data(const void* data, VkExtent3D extent, 
        TextureHandle dst) {	
    size_t data_size = extent.depth * extent.width * extent.height * 4;
    BufferHandle staging_handle = m_rm.create_buffer({
        .size = data_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_ONLY,
        .allocation_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
    });

    BufferResource* staging = m_rm.get_buffer(staging_handle);
    memcpy(staging->mapped_data, data, data_size);

    UploadJobHandle handle(m_next_handle_id++, 0);
    
    m_pending_uploads[handle] = {
        staging_handle,
        TextureUploadInfo {
            .dst = dst,
            .extent = extent
        }
    };

    return handle;


}

void UploadManager::transition_images(std::vector<TextureHandle>& handles) {
    std::vector<VkImageMemoryBarrier2> barriers;

    for(TextureHandle h : handles) {
        TextureResource* dst_ptr = m_rm.get_texture(h);

        barriers.emplace_back(VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = dst_ptr->image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        });
    }

    VkDependencyInfo dep_info {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    };

    vkCmdPipelineBarrier2(m_cmd_buffer, &dep_info);
}

void UploadManager::copy_buffer(BufferHandle src, BufferHandle dst,
        uint32_t size) {
    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    BufferResource* src_ptr = m_rm.get_buffer(src);
    BufferResource* dst_ptr = m_rm.get_buffer(dst);

    vkCmdCopyBuffer(m_cmd_buffer,
            src_ptr->buffer,
            dst_ptr->buffer,
            1, &copy_region);
}

void UploadManager::copy_buffer_to_texture(BufferHandle src, TextureHandle dst, 
        VkExtent3D extent) {
    BufferResource* src_ptr = m_rm.get_buffer(src);
    TextureResource* dst_ptr = m_rm.get_texture(dst);

    VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageExtent = extent
    };

    // copy the buffer into the image
    vkCmdCopyBufferToImage(m_cmd_buffer, src_ptr->buffer, dst_ptr->image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    
}


void UploadManager::submit_batch() {
    if(m_pending_uploads.empty()) return;
    std::cout << m_pending_uploads.size() << std::endl;

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(m_cmd_buffer, &begin_info);
    

    std::vector<TextureHandle> dst_textures;
    for(auto [handle, upload] : m_pending_uploads) {
        if(upload.is_texture()) {
            const TextureUploadInfo& tex_info = upload.as_texture();
            dst_textures.push_back(tex_info.dst);
        }
    }
    transition_images(dst_textures);

    for(auto [handle, upload] : m_pending_uploads) {
        if(upload.is_buffer()) {            
            const BufferUploadInfo& buf_info = upload.as_buffer();
            copy_buffer(upload.src, buf_info.dst, buf_info.size);
        } else if(upload.is_texture()) {
            const TextureUploadInfo& tex_info = upload.as_texture();
            copy_buffer_to_texture(upload.src, tex_info.dst, tex_info.extent); 
        }

        m_in_flight[handle] = {
            .batch_id = m_next_value,
            .staging = upload.src
        };
    }

    vkEndCommandBuffer(m_cmd_buffer);

    VkCommandBufferSubmitInfo cmd_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = m_cmd_buffer
    };

    VkSemaphoreSubmitInfo signal_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_timeline_semaphore,
        .value = m_next_value,
        .stageMask = VK_PIPELINE_STAGE_2_COPY_BIT
    };
    
    VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signal_info
    };

    vkQueueSubmit2(m_transfer_queue, 1, &submit_info, VK_NULL_HANDLE);

    m_pending_uploads.clear();

    m_last_submit = m_next_value;
    m_next_value++;
}


void UploadManager::process_completions() {
    if(m_in_flight.empty()) return;

    uint64_t last_completed = get_completed_batch_id();

    auto itr = m_in_flight.begin();
    while(itr != m_in_flight.end()) {
        if(itr->second.batch_id <= last_completed) {
            m_rm.destroy_buffer(itr->second.staging);

            itr = m_in_flight.erase(itr);
        } else {
            ++itr;
        }
    }
}

uint64_t UploadManager::get_completed_batch_id() {
    uint64_t completed;
    vkGetSemaphoreCounterValue(m_device, m_timeline_semaphore, &completed);
    return completed;
}


void UploadManager::wait_for_handle(UploadJobHandle handle) {
    auto itr = m_in_flight.find(handle);
    if(itr == m_in_flight.end()) {
        // TODO: DEBUG CHECK
        if(m_pending_uploads.find(handle) != m_pending_uploads.end()) {
            std::cerr << "did not submit before waiting" << std::endl;
        }
        return;
    }

    VkSemaphoreWaitInfo wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &m_timeline_semaphore,
        .pValues = &itr->second.batch_id
    };

    vkWaitSemaphores(m_device, &wait_info, UINT64_MAX);
}

void UploadManager::cleanup() {
    vkDestroyCommandPool(
            m_device,
            m_cmd_pool,
            nullptr);

    vkDestroySemaphore(m_device, m_timeline_semaphore, nullptr);
}
