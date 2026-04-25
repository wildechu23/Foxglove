#include "foxglove/resources/upload_manager.h"

#include <iostream>
#include <cstring>

void UploadManager::init(VulkanContext* ctx, ResourceManager* rm)  {
    m_rm = rm;

    m_device = ctx->get_device();
    m_allocator = ctx->get_allocator();
    m_transfer_queue = ctx->get_transfer_queue();

    // init commands
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->get_transfer_queue_family()
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
    BufferHandle staging_handle = m_rm->create_buffer({
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_ONLY,
        .allocation_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
    });

    // TODO: WRITE FUNCTION TO GET PTR
    BufferResource* staging = m_rm->get_buffer(staging_handle);
    memcpy(staging->mapped_data, data, size);

    UploadJobHandle handle(m_next_handle_id++, 0);
    
    m_pending_uploads[handle] = {
        .src = staging_handle,
        .dst = dst,
        .size = size,
    };

    return handle;
}


void UploadManager::submit_batch() {
    if(m_pending_uploads.empty()) return;
    std::cout << m_pending_uploads.size() << std::endl;

     VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(m_cmd_buffer, &begin_info);

    for(auto [handle, upload] : m_pending_uploads) {
        VkBufferCopy copy_region = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = upload.size
        };
        
        BufferResource* src = m_rm->get_buffer(upload.src);
        BufferResource* dst = m_rm->get_buffer(upload.dst);
        
        /*
        std::cout << "src: " << src << std::endl;
        std::cout << "dst: " << dst << std::endl;
        */
    
        vkCmdCopyBuffer(m_cmd_buffer,
                src->buffer,
                dst->buffer,
                1, &copy_region);

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
            m_rm->destroy_buffer(itr->second.staging);

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
