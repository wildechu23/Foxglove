#pragma once

#include "foxglove/resources/pipeline.h"
#include "foxglove/resources/shader_library.h"

#include "spirv_reflect.h"

#include <map>

class PipelineManager {
public:
    ComputePipeline* create_compute_pipeline(ComputeShader* shader) {
        ComputePipeline* pipeline = new ComputePipeline(m_device, shader);

        // create descriptorsetlayouts
        std::vector<DescriptorSetLayout> layouts = ShaderLibrary::reflect_layout(shader);
        // TODO: CONSIDER STDARRAY SIZE 4 INSTEAD
        pipeline->m_descriptor_layouts.reserve(layouts.size());
        for(size_t i = 0; i < layouts.size(); ++i) {
            DescriptorSetLayout& layout = layouts[i];

            VkDescriptorSetLayoutCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = layout.bindings.size(),
                .pBindings = layouts.bindings.data()
            };
            
            // TODO: MAKE IT MATCH SETNUMBER?
			vkCreateDescriptorSetLayout(m_device, create_info, nullptr, 
                    pipeline->m_descriptor_layouts[i]);
        }

        VkPipelineLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = layouts.size(),
            .pSetLayouts = pipeline->m_descriptor_layouts.data()
        };

        vkCreatePipelineLayout(m_device, &layout_info, nullptr, 
                &pipeline->m_pipeline_layout);

        VkPipelineShaderStageCreateInfo stage_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader->m_module,
            .pName = "main"
        };

        VkComputePipelineCreateInfo compute_pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .stage = stage_info,
            .layout = pipeline->m_layout
        };
        
        // TODO: consider bundling them i dunno
        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, 
                &compute_pipeline_info, nullptr, pipeline->m_pipeline);

        m_compute_pipeline_cache[shader] = pipeline;
        return pipeline;
    }
    

    // TODO: THIS NEEDS TO BE EXTENDED TO MULTIPLE SHADERS
    GraphicsPipeline* create_graphics_pipeline(GraphicsShader* shader) {

    }

    // TODO: MAKE GETTERS THAT CHECK CACHE FIRST
    ComputePipeline* get_pipeline(ComputeShader* shader) {
        // cache check
        if(m_compute_pipeline_cache.contains(shader)) {
            return m_compute_pipeline_cache[shader];
        }
        return create_compute_pipeline(shader);
    }


private:
    VkDevice m_device;

    // pipeline caches, will need to be thread safe eventually
    std::unordered_map<ComputeShader*, ComputePipeline*> m_compute_pipeline_cache;
};
