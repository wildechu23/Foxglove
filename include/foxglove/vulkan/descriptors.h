#pragma once

#include "foxglove/resources/resource_manager.h"
#include "foxglove/framegraph/util.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <span>
#include <stdexcept>

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(
			VkDevice device,
			VkShaderStageFlags shaderStages,
			void* pNext = nullptr,
			VkDescriptorSetLayoutCreateFlags flags = 0
	);
};

// consider VK_EXT_DESCRIPTOR_BUFFER
struct DescriptorAllocator {
    struct PoolSizeRatio{
		VkDescriptorType type;
		float ratio;
    };

    VkDescriptorPool pool;

    void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
    std::vector<VkDescriptorSet> allocate(VkDevice device, std::vector<VkDescriptorSetLayout>& layouts);
};

struct HeapInfo {
    BufferHandle handle;
    void* mapped;
    VkDeviceAddress address;
    VkDeviceSize size;
    VkDeviceSize offset;
};

// SPLIT THE HEAP UP INTO SECTIONS
struct SectionInfo {
    uint32_t start_offset;
    uint32_t size;
    uint32_t current_offset;
    uint32_t descriptor_size;
    uint32_t alignment;
};

struct BufferDescriptorInfo {
    BufferHandle resource;
    BufferUsage usage;
};

struct TextureDescriptorInfo {
    TextureHandle resource;
    TextureUsage usage;
};


class DescriptorHeapAllocator {
public:
    DescriptorHeapAllocator() = default;
    ~DescriptorHeapAllocator() = default;

    struct Slot {
        uint32_t gen = 0;
        ResourceType type;
        uint32_t offset;
    };

    void init(VulkanContext* ctx, ResourceManager* rm);
    
    void add_descriptors(std::vector<BufferDescriptorInfo>& new_buffers, 
                         std::vector<TextureDescriptorInfo>& new_textures) {
        buffers.insert(std::end(buffers), std::begin(new_buffers), 
                std::end(new_buffers));
        textures.insert(std::end(textures), std::begin(new_textures), 
                std::end(new_textures));
    }

    bool has_resource(Handle h) {
        return handle_to_slot.contains(h);
    }

    uint32_t get_index(BufferHandle h) {
        return slots[handle_to_slot[h]].offset / m_buffer_descriptor_size;
    }

    uint32_t get_index(TextureHandle h) {
        return slots[handle_to_slot[h]].offset / m_image_descriptor_size;
    }
    
    // update once a frame?
    void write_pending(VkDevice device);
private:
    uint32_t allocate_section(Handle handle, uint32_t section);

    uint32_t get_free_slot() {
        slots.emplace_back();
        return slots.size() - 1;
    }

    ResourceManager* m_rm;
    
    HeapInfo m_resource_heap;
    std::vector<SectionInfo> sections;


    uint32_t resource_count = 0;
    // TODO: MOVE TO A BETTER PLACE
    // another one somewhere else i think pass context
	PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptorsEXT{ nullptr };

    std::vector<BufferDescriptorInfo> buffers;
    std::vector<TextureDescriptorInfo> textures;
    
    std::vector<Slot> slots;
    std::unordered_map<Handle, uint32_t> handle_to_slot;
    
    uint8_t* heap_address;

    uint32_t m_reserved_size;
    uint32_t m_buffer_descriptor_size;
    uint32_t m_image_descriptor_size;

    uint32_t m_buffer_align;
    uint32_t m_image_align;
    
};
