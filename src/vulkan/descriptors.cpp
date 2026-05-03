#include "foxglove/vulkan/descriptors.h"

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newbind {};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
        VkDevice device,
        VkShaderStageFlags shaderStages,
        void* pNext,
        VkDescriptorSetLayoutCreateFlags flags
) {
    for (auto& b : bindings) {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = (uint32_t)bindings.size(),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout set;
    if(vkCreateDescriptorSetLayout(device, &info, nullptr, &set) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    return set;
}


void DescriptorAllocator::init_pool(
        VkDevice device, 
        uint32_t maxSets, 
        std::span<PoolSizeRatio> poolRatios
) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = maxSets,
        .poolSizeCount = (uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data()
    };

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device) {
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device) {
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet ds;
    if(vkAllocateDescriptorSets(device, &allocInfo, &ds) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    return ds;
}


std::vector<VkDescriptorSet> DescriptorAllocator::allocate(VkDevice device, std::vector<VkDescriptorSetLayout>& layouts) {
    if(layouts.size() == 0) {
        return {};
    }

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    std::vector<VkDescriptorSet> ds(layouts.size());
    if(vkAllocateDescriptorSets(device, &allocInfo, ds.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    return ds;
}

uint32_t align(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void DescriptorHeapAllocator::init(VulkanContext* ctx, ResourceManager* rm) {
    m_rm = rm;

    VkDevice device = ctx->get_device();
    VkPhysicalDevice physical_device = ctx->get_physical_device();
		
	vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));

    VkPhysicalDeviceDescriptorHeapPropertiesEXT heap_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT
    };
    
    VkPhysicalDeviceProperties2 props2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &heap_props
    };
    vkGetPhysicalDeviceProperties2(physical_device, &props2);

    m_buffer_align = heap_props.bufferDescriptorAlignment;
    m_image_align = heap_props.imageDescriptorAlignment;

    m_reserved_size = heap_props.minResourceHeapReservedRange;
    m_buffer_descriptor_size = align(heap_props.bufferDescriptorSize,
            m_buffer_align);
    m_image_descriptor_size = align(heap_props.imageDescriptorSize,
            m_image_align);
    
    uint32_t persistent_images = 50;
    uint32_t transient_images = 50;
    uint32_t persistent_buffers = 50;
    uint32_t transient_buffers = 50;

    uint32_t offset = 0;

    sections.resize(4);

    // Persistent images
    sections[0] = {
        .start_offset = offset,
        .size = persistent_images * m_image_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_image_descriptor_size,
        .alignment = m_image_align
    };
    offset = align(offset + sections[0].size, m_image_descriptor_size);
    
    // Transient images
    sections[1] = {
        .start_offset = offset,
        .size = transient_images * m_image_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_image_descriptor_size,
        .alignment = m_image_align
    };
    offset = align(offset + sections[1].size, m_buffer_descriptor_size);
    
    // Persistent buffers
    sections[2] = {
        .start_offset = offset,
        .size = persistent_buffers * m_buffer_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_buffer_descriptor_size,
        .alignment = m_buffer_align
    };
    offset = align(offset + sections[2].size, m_buffer_descriptor_size);
    
    // Transient buffers
    sections[3] = {
        .start_offset = offset,
        .size = transient_buffers * m_buffer_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_buffer_descriptor_size,
        .alignment = m_buffer_align
    };

    // heap_size
    offset += sections.back().size;

    uint32_t heap_size = offset;
    std::cout << heap_size << std::endl;
        
    BufferHandle rh_buffer = m_rm->create_buffer(BufferDesc{
        .size = heap_size,
        .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_usage = VMA_MEMORY_USAGE_AUTO,
        .allocation_flags = 
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
          | VMA_ALLOCATION_CREATE_MAPPED_BIT
    });

    BufferResource* rh_resource = m_rm->get_buffer(rh_buffer); 

    m_resource_heap = HeapInfo{
        .handle = rh_buffer,
        .mapped = rh_resource->mapped_data,
        .address = m_rm->_get_buffer_address(rh_resource),
        .size = heap_size,
        .offset = 0
    };
    
    heap_address = static_cast<uint8_t*>(m_resource_heap.mapped);

    // TODO: IGNORE SAMPLER HEAP FOR NOW
}

uint32_t DescriptorHeapAllocator::allocate_section(Handle handle,
        uint32_t section) {
    SectionInfo& info = sections[section];

    if(info.current_offset == info.start_offset + info.size) {
        std::cerr << "full section" << std::endl; 
    } else if(info.current_offset > info.start_offset + info.size) {
        std::cerr << "somehow offset unaligned" << std::endl;
    }

    uint32_t slot = get_free_slot();
    
    slots[slot] = {
        .gen = slots[slot].gen + 1,
        .type = ResourceType::Buffer,
        .offset = info.start_offset + info.current_offset
    };
    
    // offset is descriptor_size agnostic (pure offset)
    info.current_offset += info.descriptor_size;
    info.current_offset = align(info.current_offset, info.alignment);

    handle_to_slot[handle] = slot;
    return slot;
}
 
void DescriptorHeapAllocator::write_pending(VkDevice device) {
    if(buffers.empty() && textures.empty()) return;

    std::vector<VkDeviceAddressRangeEXT> device_address_ranges;
    std::vector<VkResourceDescriptorInfoEXT> resource_descriptor_infos;
    std::vector<VkHostAddressRangeEXT> host_address_ranges;

    std::vector<VkImageViewCreateInfo> image_view_infos;
    std::vector<VkImageDescriptorInfoEXT> image_descriptor_infos;

    for(size_t i = 0; i < buffers.size(); ++i) {
        BufferDescriptorInfo info = buffers[i];
        BufferResource* resource = m_rm->get_buffer(info.resource);
        
        uint32_t slot = allocate_section(info.resource, 3);

        device_address_ranges.push_back({
            .address = m_rm->_get_buffer_address(resource),
            .size = resource->size
        });

        resource_descriptor_infos.push_back({
            .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .type = util::deduce_descriptor_type(info.usage),
            .data = { .pAddressRange = &device_address_ranges.back() }
        });

        host_address_ranges.push_back({
            .address = heap_address + slots[slot].offset,
            .size = m_buffer_descriptor_size
        });
    }

    for(size_t i = 0; i < textures.size(); ++i) {
        TextureDescriptorInfo info = textures[i];
        TextureResource* resource = m_rm->get_texture(info.resource);

        uint32_t slot = allocate_section(info.resource, 1);
        
        image_view_infos.push_back(resource->get_view_create_info());
        image_descriptor_infos.push_back({
            .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
            .pView = &image_view_infos.back(),
            .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL //TODO: CORRECT?
        });

        resource_descriptor_infos.push_back({
            .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .type = util::deduce_descriptor_type(info.usage),
            .data = { .pImage = &image_descriptor_infos.back() }
        });

        host_address_ranges.push_back({
            .address = heap_address + slots[slot].offset,
            .size = m_image_descriptor_size
        });
    }

    vkWriteResourceDescriptorsEXT(device, 
        static_cast<uint32_t>(resource_descriptor_infos.size()),
        resource_descriptor_infos.data(), 
        host_address_ranges.data()
    );

    resource_count += buffers.size() + textures.size();
    buffers.clear();
    textures.clear();
}
