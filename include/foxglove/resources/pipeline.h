#pragma once

#include "shader.h"

class Pipeline {
public:
    Pipeline(VkDevice device) : m_device(device) {}
protected:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;
    std::vector<VkDescriptorSetLayout> m_descriptor_layouts;

    friend class PipelineManager;
};

class ComputePipeline : public Pipeline { 
public:
    ComputePipeline(VkDevice device, ComputeShader* shader) : 
        Pipeline(device), m_shader(shader) {}

private:
    ComputeShader* m_shader;

};
