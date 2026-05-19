#include "foxglove/vulkan/descriptors.h"
#include "foxglove/core/math.h"

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

/*
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
*/

//
// DescriptorHeapAllocator
//

void DescriptorHeapAllocator::init(VulkanContext* ctx, ResourceManager* rm) {
    m_ctx = ctx;
    m_rm = rm;

    VkPhysicalDevice physical_device = ctx->get_physical_device();
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
    m_sampler_align = heap_props.samplerDescriptorAlignment;

    m_resource_reserved_size = heap_props.minResourceHeapReservedRange;
    m_sampler_reserved_size = heap_props.minSamplerHeapReservedRange;

    m_buffer_descriptor_size = align_up(heap_props.bufferDescriptorSize,
            m_buffer_align);
    m_image_descriptor_size = align_up(heap_props.imageDescriptorSize,
            m_image_align);
    m_sampler_descriptor_size = align_up(heap_props.samplerDescriptorSize,
            m_sampler_align);
    
    uint32_t persistent_images = 50;
    uint32_t transient_images = 50;
    uint32_t persistent_buffers = 50;
    uint32_t transient_buffers = 50;

    uint32_t offset = 0;

    std::vector<SectionInfo> sections;
    sections.resize(4);

    // Persistent images
    sections[0] = {
        .start_offset = offset,
        .size = persistent_images * m_image_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_image_descriptor_size,
        .alignment = m_image_align
    };
    offset = align_up(offset + sections[0].size, m_image_descriptor_size);
    
    // Transient images
    sections[1] = {
        .start_offset = offset,
        .size = transient_images * m_image_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_image_descriptor_size,
        .alignment = m_image_align
    };
    offset = align_up(offset + sections[1].size, m_buffer_descriptor_size);
    
    // Persistent buffers
    sections[2] = {
        .start_offset = offset,
        .size = persistent_buffers * m_buffer_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_buffer_descriptor_size,
        .alignment = m_buffer_align
    };
    offset = align_up(offset + sections[2].size, m_buffer_descriptor_size);
    
    // Transient buffers
    sections[3] = {
        .start_offset = offset,
        .size = transient_buffers * m_buffer_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_buffer_descriptor_size,
        .alignment = m_buffer_align
    };
    offset += sections.back().size;

    uint32_t resource_heap_size = offset + m_resource_reserved_size;
    m_resource_heap.init(sections, resource_heap_size, m_rm);

    offset = 0;
    
    // sampler heap
    std::vector<SectionInfo> sampler_sections(1);
    sampler_sections[0] =  {
        .start_offset = offset,
        .size = 20 * m_sampler_descriptor_size,
        .current_offset = 0,
        .descriptor_size = m_sampler_descriptor_size,
        .alignment = m_sampler_align
    };
    offset += sampler_sections[0].size;

    uint32_t sampler_heap_size = offset + m_sampler_reserved_size;
    m_sampler_heap.init(sampler_sections, sampler_heap_size, m_rm);
}

void DescriptorHeapAllocator::add_descriptors(
        std::vector<BufferKey>& new_buffers, 
        std::vector<TextureKey>& new_textures,
        std::vector<SamplerKey>& new_samplers,
        std::vector<BufferKey>& new_transient_buffers,
        std::vector<TextureKey>& new_transient_textures) {
    buffers.insert(std::end(buffers), std::begin(new_buffers), 
            std::end(new_buffers));
    textures.insert(std::end(textures), std::begin(new_textures), 
            std::end(new_textures));
    samplers.insert(std::end(samplers), std::begin(new_samplers),
            std::end(new_samplers));

    transient_buffers.insert(std::end(transient_buffers), 
            std::begin(new_transient_buffers),
            std::end(new_transient_buffers));
    transient_textures.insert(std::end(transient_textures),
            std::begin(new_transient_textures), 
            std::end(new_transient_textures));
}

void DescriptorHeapAllocator::write_pending() {
    write_pending_resources();
    write_pending_samplers();
}

void DescriptorHeapAllocator::write_pending_resources() {
    if(buffers.empty() && textures.empty() &&
            transient_buffers.empty() && transient_textures.empty()) return;

    size_t total_buffers = buffers.size() + transient_buffers.size();
    size_t total_textures = textures.size() + transient_textures.size();

    std::vector<VkDeviceAddressRangeEXT> device_address_ranges;
    std::vector<VkResourceDescriptorInfoEXT> resource_descriptor_infos;
    std::vector<VkHostAddressRangeEXT> host_address_ranges;

    std::vector<VkImageViewCreateInfo> image_view_infos;
    std::vector<VkImageDescriptorInfoEXT> image_descriptor_infos;
    
    // TODO: RESERVE PROPER SIZE
    device_address_ranges.reserve(total_buffers);
    resource_descriptor_infos.reserve(total_buffers);

    image_view_infos.reserve(total_textures);
    image_descriptor_infos.reserve(total_textures);

    auto process_buffers = [&](const auto& buffer_list, uint32_t section) {
        for(const auto& key : buffer_list) {
            BufferResource* resource = m_rm->get_buffer(key.handle);
            uint32_t slot = buffer_manager.allocate_section(
                m_resource_heap.get_section(section),
                key,
                m_resource_heap.get_free_slot()
            );

            device_address_ranges.push_back({
                .address = m_rm->_get_buffer_address(resource),
                .size = resource->size
            });

            resource_descriptor_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .type = util::deduce_descriptor_type(key.usage),
                .data = { .pAddressRange = &device_address_ranges.back() }
            });

            host_address_ranges.push_back({
                .address = m_resource_heap.get_address() 
                           + m_resource_heap.slots[slot].offset,
                .size = m_buffer_descriptor_size
            });
        }
    };

    auto process_textures = [&](const auto& texture_list, uint32_t section) {
        for(const auto& key : texture_list) {
            TextureResource* resource = m_rm->get_texture(key.handle);
            uint32_t slot = texture_manager.allocate_section(
                m_resource_heap.get_section(section),
                key,
                m_resource_heap.get_free_slot()
            );
            
            image_view_infos.push_back(resource->get_view_create_info());
            image_descriptor_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
                .pView = &image_view_infos.back(),
                .layout = util::deduce_layout(key.usage),
            });
            // TODO: DEDUCE WITH ACCESS

            resource_descriptor_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .type = util::deduce_descriptor_type(key.usage),
                .data = { .pImage = &image_descriptor_infos.back() }
            });

            host_address_ranges.push_back({
                .address = m_resource_heap.get_address() 
                            + m_resource_heap.slots[slot].offset,
                .size = m_image_descriptor_size
            });
        }
    };

    process_buffers(buffers, 2);
    process_buffers(transient_buffers, 3);
    process_textures(textures, 0);
    process_textures(transient_textures, 1);
    
    m_ctx->vkWriteResourceDescriptorsEXT(m_ctx->get_device(), 
        static_cast<uint32_t>(resource_descriptor_infos.size()),
        resource_descriptor_infos.data(), 
        host_address_ranges.data()
    );

    buffers.clear();
    textures.clear();
    transient_buffers.clear();
    transient_textures.clear();
}

void DescriptorHeapAllocator::write_pending_samplers() {
    if(samplers.empty()) return;

    std::vector<VkSamplerCreateInfo> sampler_infos;
    std::vector<VkHostAddressRangeEXT> sampler_host_address_ranges;

    sampler_infos.reserve(samplers.size());

    for(size_t i = 0; i < samplers.size(); ++i) {
        SamplerKey key = samplers[i];
        SamplerResource* resource = m_rm->get_sampler(key);

        uint32_t section = 0;
        uint32_t slot = sampler_manager.allocate_section(
            m_sampler_heap.get_section(section),
            key,
            m_sampler_heap.get_free_slot());

        sampler_infos.push_back(resource->info);
        sampler_host_address_ranges.push_back({
            .address = m_sampler_heap.get_address()
                        + m_sampler_heap.slots[slot].offset,
            .size = m_sampler_descriptor_size
        });
    }

    m_ctx->vkWriteSamplerDescriptorsEXT(m_ctx->get_device(),
        static_cast<uint32_t>(sampler_infos.size()),
        sampler_infos.data(),
        sampler_host_address_ranges.data()
    );

    samplers.clear();
}

void DescriptorHeapAllocator::bind_resource_heap(VkCommandBuffer cmd) {
    HeapInfo& rinfo = m_resource_heap.m_heap;
    VkBindHeapInfoEXT bind_resource_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange{
            .address = rinfo.address,
            .size = rinfo.size
        },
        .reservedRangeOffset = rinfo.size - m_resource_reserved_size,
        .reservedRangeSize = m_resource_reserved_size 
    };

    m_ctx->vkCmdBindResourceHeapEXT(cmd, &bind_resource_heap_info);
}

void DescriptorHeapAllocator::bind_sampler_heap(VkCommandBuffer cmd) {
    HeapInfo& sinfo = m_sampler_heap.m_heap;
    VkBindHeapInfoEXT bind_sampler_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange{
            .address = sinfo.address,
            .size = sinfo.size
        },
        .reservedRangeOffset = sinfo.size - m_sampler_reserved_size,
        .reservedRangeSize = m_sampler_reserved_size 
    };

    m_ctx->vkCmdBindSamplerHeapEXT(cmd, &bind_sampler_heap_info); 
}
