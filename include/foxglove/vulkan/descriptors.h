#pragma once

#include "foxglove/core/math.h"
#include "foxglove/resources/resource_manager.h"
#include "foxglove/framegraph/util.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <variant>
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

/*
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
*/

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

struct Slot {
    uint32_t gen = 0;
    ResourceType type;
    uint32_t offset;
};

template<typename KeyT>
class DescriptorTypeManager {
public:
    DescriptorTypeManager(std::vector<Slot>& s, std::vector<uint32_t>& f) 
        : slots(s), free_slots(f) {}

    bool has_resource(const KeyT& key) { 
        return key_to_slot.contains(key); 
    }

    Slot& get_slot(const KeyT& key) { 
        return slots[key_to_slot[key]]; 
    }

    uint32_t get_offset(const KeyT& key) { 
        return get_slot(key).offset; 
    }

    void free_key(const KeyT& key) {
        auto itr = key_to_slot.find(key);
        if(itr == key_to_slot.end()) return;

        free_slots.push_back(itr->second);
        key_to_slot.erase(itr);
    }
    
    uint32_t allocate_section(SectionInfo& info, KeyT key, uint32_t slot) {
        if(info.current_offset == info.start_offset + info.size) {
            std::cerr << "full section" << std::endl; 
            // for now
            info.current_offset = 0;
        } else if(info.current_offset > info.start_offset + info.size) {
            std::cerr << "somehow offset unaligned" << std::endl;
        }

        //uint32_t slot = get_free_slot();

        slots[slot] = {
            .gen = slots[slot].gen + 1,
            .type = ResourceType::Buffer, 
            .offset = info.start_offset + info.current_offset
        };

        // offset is descriptor_size-agnostic (pure offset)
        info.current_offset += info.descriptor_size;
        info.current_offset = align_up(info.current_offset, info.alignment);

        key_to_slot[key] = slot;
        return slot;
    }

protected:
    std::unordered_map<KeyT, uint32_t> key_to_slot;
    std::vector<Slot>& slots;
    std::vector<uint32_t>& free_slots;
};

struct BufferKey {
    BufferHandle handle;
    BufferUsage usage;
    bool operator==(const BufferKey& other) const = default;
};

struct TextureKey {
    TextureHandle handle;
    TextureUsage usage;
    ResourceAccess access;
    bool operator==(const TextureKey& other) const = default;
};

template<>
struct std::hash<BufferKey> {
    size_t operator()(const BufferKey& key) const {
        size_t h1 = std::hash<BufferHandle>{}(key.handle);
        size_t h2 = std::hash<size_t>{}(static_cast<size_t>(key.usage));
        return h1 ^ (h2 << 1);
    }
};

template<>
struct std::hash<TextureKey> {
    size_t operator()(const TextureKey& key) const {
        size_t h1 = hash<TextureHandle>{}(key.handle);
        size_t h2 = std::hash<size_t>{}(static_cast<size_t>(key.usage));
        size_t h3 = std::hash<size_t>{}(static_cast<size_t>(key.access));
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class DescriptorHeap {
public:    
    void init(std::vector<SectionInfo>& sections, 
            uint32_t heap_size, ResourceManager* rm) {
        m_sections = sections;

        BufferHandle heap_buffer = rm->create_buffer(BufferDesc{
                .size = heap_size,
                .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .memory_usage = VMA_MEMORY_USAGE_AUTO,
                .allocation_flags = 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT
                });

        BufferResource* heap_resource = rm->get_buffer(heap_buffer); 

        m_heap = HeapInfo{
            .handle = heap_buffer,
            .mapped = heap_resource->mapped_data,
            .address = rm->_get_buffer_address(heap_resource),
            .size = heap_size,
            .offset = 0
        };

        heap_address = static_cast<uint8_t*>(m_heap.mapped);
    }

    uint8_t* get_address() const { return heap_address; }
    SectionInfo& get_section(uint32_t i) { return m_sections[i]; }
private:

    uint32_t get_free_slot() {
        if(!free_slots.empty()) {
            uint32_t idx = free_slots.back();
            free_slots.pop_back();
            return idx;
        }

        slots.emplace_back();

        return slots.size() - 1;
    }

    HeapInfo m_heap;
    std::vector<SectionInfo> m_sections;

    std::vector<Slot> slots;
    std::vector<uint32_t> free_slots;

    uint8_t* heap_address;

    friend class DescriptorHeapAllocator;
};


struct BufferDescriptorInfo {
    BufferHandle resource;
    BufferUsage usage;
    bool transient;
};

struct TextureDescriptorInfo {
    TextureHandle resource;
    TextureUsage usage;
    ResourceAccess access;
    bool transient;
};

struct SamplerDescriptorInfo {
    SamplerHandle resource;
    // bool transient;
};

class DescriptorHeapAllocator {
public:
    DescriptorHeapAllocator() = default;
    ~DescriptorHeapAllocator() = default;
    void init(VulkanContext* ctx, ResourceManager* rm);
    
    void add_descriptors(
            std::vector<BufferDescriptorInfo>& new_buffers, 
            std::vector<TextureDescriptorInfo>& new_textures,
            std::vector<SamplerDescriptorInfo>& new_samplers
    ) {
        buffers.insert(std::end(buffers), std::begin(new_buffers), 
                std::end(new_buffers));
        textures.insert(std::end(textures), std::begin(new_textures), 
                std::end(new_textures));
        samplers.insert(std::end(samplers), std::begin(new_samplers),
                std::end(new_samplers));
    }
    
    bool has_resource(BufferKey h) { return buffer_manager.has_resource(h); }
    bool has_resource(TextureKey h) { return texture_manager.has_resource(h); }
    bool has_resource(SamplerHandle h) { return sampler_manager.has_resource(h); }


    uint32_t get_index(BufferKey h){ 
        return buffer_manager.get_offset(h) / m_buffer_descriptor_size;
    }

    uint32_t get_index(TextureKey h) {
        return texture_manager.get_offset(h) / m_image_descriptor_size;
    }

    uint32_t get_index(SamplerHandle h) {
        return sampler_manager.get_offset(h) / m_sampler_descriptor_size;
    }

    // update once a frame?
    void write_pending();
    void bind_resource_heap(VkCommandBuffer cmd);
    void bind_sampler_heap(VkCommandBuffer cmd);

    void free_resource(BufferKey key)   { buffer_manager.free_key(key); }
    void free_resource(TextureKey key)  { texture_manager.free_key(key); }
    void free_resource(SamplerHandle h) { sampler_manager.free_key(h); }
private:
    void write_pending_resources();
    void write_pending_samplers();
    
    VulkanContext* m_ctx;
    ResourceManager* m_rm;

    DescriptorHeap m_resource_heap;
    DescriptorHeap m_sampler_heap;

    DescriptorTypeManager<BufferKey> buffer_manager{
        m_resource_heap.slots, m_resource_heap.free_slots};
    DescriptorTypeManager<TextureKey> texture_manager{
        m_resource_heap.slots, m_resource_heap.free_slots};
    DescriptorTypeManager<SamplerHandle> sampler_manager{
        m_sampler_heap.slots, m_sampler_heap.free_slots};

    std::vector<BufferDescriptorInfo> buffers;
    std::vector<TextureDescriptorInfo> textures;
    std::vector<SamplerDescriptorInfo> samplers;
    
    uint32_t m_resource_reserved_size;
    uint32_t m_sampler_reserved_size;

    uint32_t m_buffer_descriptor_size;
    uint32_t m_image_descriptor_size;
    uint32_t m_sampler_descriptor_size;

    uint32_t m_buffer_align;
    uint32_t m_image_align;
    uint32_t m_sampler_align;
    
};
