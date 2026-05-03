#pragma once

#include "foxglove/core/types.h"
#include "foxglove/core/handle.h"
#include "foxglove/vulkan/vulkan_context.h"

#include "foxglove/resources/resource.h"
#include "foxglove/resources/resource_manager.h"

using UploadJobHandle = TaggedHandle<JobType::Upload>; 

class UploadManager {
public:
    UploadManager(VulkanContext& ctx, ResourceManager& rm);

    void init();
    void cleanup();

    UploadJobHandle upload_data(const void* data, size_t size, 
            BufferHandle dst);
    void copy_buffer(BufferHandle src, BufferHandle dst);

    void submit_batch();
    void process_completions();

    void wait_for_handle(UploadJobHandle handle);

private:
    uint64_t get_completed_batch_id();

    VkDevice m_device;
    VmaAllocator m_allocator;

    VkCommandPool m_cmd_pool;
    VkCommandBuffer m_cmd_buffer;
    //VkFence m_fence;

    VkQueue m_transfer_queue;
    uint32_t m_transfer_queue_family;
    
    VkSemaphore m_timeline_semaphore;

    struct PendingUpload {
        BufferHandle src;
        BufferHandle dst;
        size_t size;
    };

    struct InFlightUpload {
        uint64_t batch_id;
        BufferHandle staging;
    };

    std::unordered_map<UploadJobHandle, PendingUpload> m_pending_uploads;
    std::unordered_map<UploadJobHandle, InFlightUpload> m_in_flight;

    uint64_t m_last_submit = 0;
    uint64_t m_next_value = 1;

    uint64_t m_next_handle_id;

    ResourceManager& m_rm;
};
