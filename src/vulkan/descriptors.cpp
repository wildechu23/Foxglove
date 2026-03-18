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
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = layouts.size(),
        .pSetLayouts = layouts.data()
    };

    std::vector<VkDescriptorSet> ds(layouts.size());
    if(vkAllocateDescriptorSets(device, &allocInfo, ds.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    return std::move(ds);
}
