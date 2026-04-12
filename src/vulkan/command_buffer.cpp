#include "foxglove/vulkan/command_buffer.h"

/*
void CommandContext::bind_compute_pipeline(ComputePipeline* pipeline) {
    m_curr_pipeline = pipeline;
}

void CommandContext::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    /*
     * 1. initialize descriptor layouts
     * 2. create compute pipeline
     * 3. set current compute pipeline
     * 4. allocate descriptor sets
     * 5. swap descriptor sets
     

	vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
            m_curr_pipeline->get_pipeline());
    

    // MAKE DESCRIPTOR BUFFER INFO AND OTHER STUFF
    // TODO: THIS FUCKING SUCKS, MAYBE HALF BUILD BEFORE WE GET HERE?
    //
    // i need a vkwritedescriptorset for each descriptor type
    
    // GET THE BINDINGS SHIT
    
    
    // TODO GET BINDINGS FROM M_PASS HERE??? I HATE THE DESIGN CHOICE THOUGH

    const std::vector<DescriptorSetLayout>& reflection = m_curr_pipeline->get_reflection();
    
    int m = reflection.size();

    // note all sets will be together in write_infos
    
    std::vector<VkWriteDescriptorSet> write_sets;
    std::vector<DescriptorWriteInfo> write_infos;
    std::vector<VkDescriptorSet> descriptor_sets(m);
    
    for(size_t i = 0; i < reflection.size(); ++i) {
        const DescriptorSetLayout& layout = reflection[i];
        const auto& bindings = layout.bindings;
        
        VkDescriptorSet descriptor_set = descriptor_sets[i];
        for(size_t j = 0; j < bindings.size(); ++j) {
            const auto& binding = bindings[j];

            switch(binding.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                    // Create buffer infos
                    std::vector<VkDescriptorBufferInfo> buffer_infos;
                    for(uint32_t k = 0; k < binding.descriptorCount; k++) {
                        FGResource* fg_r = m_curr_params[i][j][k];
                        assert(fg_r->get_type() == TypeID::Buffer);

                        FGBuffer* fg_buffer = static_cast<FGBuffer*>(fg_r);
                        BufferResource* buffer = fg_buffer->get_resource();

                        VkDescriptorBufferInfo buffer_info = {
                            .buffer = buffer->buffer, 
                            .offset = 0, 
                            .range = buffer->size
                        };
                        buffer_infos.push_back(buffer_info);
                    }

                    write_infos.emplace_back(std::move(buffer_infos));
                    DescriptorWriteInfo& write_info = write_infos.back();
                    
                    write_sets.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = descriptor_set,
                        .dstBinding = static_cast<uint32_t>(j),
                        .descriptorCount = binding.descriptorCount,
                        .descriptorType = binding.descriptorType,
                        .pBufferInfo = std::get<std::vector<VkDescriptorBufferInfo>>(write_info).data()
                    });
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                    std::vector<VkDescriptorImageInfo> image_infos;
                    for(uint32_t k = 0; k < binding.descriptorCount; k++) {
                        FGResource* fg_r = m_curr_params[i][j][k];
                        assert(fg_r->get_type() == TypeID::Texture);

                        FGTexture* fg_texture = static_cast<FGTexture*>(fg_r);
                        TextureResource* texture = fg_texture->get_resource();
                        
                        // sampler goes here too
                        VkDescriptorImageInfo image_info = {
                            .imageView = texture->view,
                            .imageLayout = util::deduce_layout(
                                    fg_texture->get_usage())
                        };
                        image_infos.push_back(image_info);
                    }
                    write_infos.emplace_back(std::move(image_infos));
                    DescriptorWriteInfo& write_info = write_infos.back();
                    
                    write_sets.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = descriptor_set,
                        .dstBinding = static_cast<uint32_t>(j),
                        .descriptorCount = binding.descriptorCount,
                        .descriptorType = binding.descriptorType,
                        .pImageInfo = std::get<std::vector<VkDescriptorImageInfo>>(write_info).data()
                    });
                    break;
                }
                default:
                    break;
            }        
        }
    }

   
    vkUpdateDescriptorSets(m_device, write_sets.size(), 
            write_sets.data(), 0, nullptr);
}
*/
