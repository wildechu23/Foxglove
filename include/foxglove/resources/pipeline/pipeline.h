#pragma once

#include "foxglove/resources/shader/shader.h"
#include "xxhash.h"

class Pipeline {
public:
    virtual ~Pipeline() { cleanup(); }

    VkPipeline get_pipeline() const { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }

    //std::vector<DescriptorSetLayout> get_reflection() const { return m_reflection; }
protected:
    VkDevice m_device;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    std::vector<VkDescriptorSetLayout> m_descriptor_layouts;
    std::vector<VkPushConstantRange> m_push_constant_ranges;
   
    Pipeline(VkDevice device) :
        m_device(device),
        m_pipeline(VK_NULL_HANDLE),
        m_pipeline_layout(VK_NULL_HANDLE) {}

    void cleanup() {
        for(auto& layout : m_descriptor_layouts) {
            vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
        }
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }

    friend class PipelineManager;
    friend class PassContext;
};

struct ComputePipelineDesc {
    ComputeShader* m_shader;

    uint64_t create_key() {
        return std::hash<ComputeShader*>{}(m_shader);
    }
};

class ComputePipeline : public Pipeline { 
public:
    using Desc = ComputePipelineDesc;

    ComputePipeline(VkDevice device, Desc desc) : 
        Pipeline(device), m_desc(desc) {}

private:
    Desc m_desc;

};

struct GraphicsShaderSet {
    VertexShader* vs;
    FragmentShader* fs;
    GeometryShader* gs;
};

struct GraphicsPipelineConfig {
    GraphicsShaderSet                       shader_set;

    VkPipelineInputAssemblyStateCreateInfo  input_assembly;
    VkPipelineRasterizationStateCreateInfo  rasterizer;
    VkPipelineColorBlendAttachmentState     color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo    multisampling;
    VkPipelineDepthStencilStateCreateInfo   depth_stencil;
    VkPipelineRenderingCreateInfo           render_info;
    VkFormat                                color_attachment_format;
};

// TODO: FINISH THESE FIELDS
struct GraphicsPipelineDesc {
    // vectors exist for dynamic states
    std::vector<VkPipelineColorBlendAttachmentState>    m_color_blend_attachments;

    // VkPipelineVertexInputStateCreateInfo
    std::vector<VkVertexInputBindingDescription>        m_vertex_bindings;
    std::vector<VkVertexInputAttributeDescription>      m_vertex_attributes;
   

    VkPipelineMultisampleStateCreateInfo                m_multisampling;

    VkPipelineInputAssemblyStateCreateInfo              m_input_assembly;
    VkPipelineRasterizationStateCreateInfo              m_rasterizer;
    VkPipelineDepthStencilStateCreateInfo               m_depth_stencil;

    // VkPipelineRenderingCreateInfo
    std::vector<VkFormat>   m_color_attachment_formats;
    VkFormat                m_depth_attachment_format = VK_FORMAT_UNDEFINED;
    VkFormat                m_stencil_attachment_format = VK_FORMAT_UNDEFINED;
    uint32_t                m_view_mask = 0;

    // skip renderpass because dynamic rendering
    
    uint32_t m_subpass_idx;

    uint64_t create_key() const {
        XXH3_state_t state;
        XXH3_64bits_reset(&state);
        
        // vectors
        XXH3_64bits_update(&state, m_color_blend_attachments.data(),
                m_color_blend_attachments.size() * 
                sizeof(VkPipelineColorBlendAttachmentState));
        XXH3_64bits_update(&state, m_vertex_bindings.data(),
                m_vertex_bindings.size() * 
                sizeof(VkVertexInputBindingDescription));
        XXH3_64bits_update(&state, m_vertex_attributes.data(),
                m_vertex_attributes.size() * 
                sizeof(VkVertexInputAttributeDescription));
        XXH3_64bits_update(&state, m_color_attachment_formats.data(),
                m_color_attachment_formats.size() * sizeof(VkFormat));
    
        // Fixed-size structs
        XXH3_64bits_update(&state, &m_multisampling, sizeof(m_multisampling));
        XXH3_64bits_update(&state, &m_input_assembly, sizeof(m_input_assembly));
        XXH3_64bits_update(&state, &m_rasterizer, sizeof(m_rasterizer));
        XXH3_64bits_update(&state, &m_depth_stencil, sizeof(m_depth_stencil));
        XXH3_64bits_update(&state, &m_depth_attachment_format, sizeof(m_depth_attachment_format));
        XXH3_64bits_update(&state, &m_stencil_attachment_format, sizeof(m_stencil_attachment_format));
        XXH3_64bits_update(&state, &m_view_mask, sizeof(m_view_mask));
        XXH3_64bits_update(&state, &m_subpass_idx, sizeof(m_subpass_idx));
        
        return XXH3_64bits_digest(&state);
    };
};

struct DrawConfig {
    VkViewport viewport;
    VkRect2D scissor;
};

class GraphicsPipeline : public Pipeline { 
public:
    using Desc = GraphicsPipelineDesc;

    GraphicsPipeline(VkDevice device, Desc desc) : 
        Pipeline(device), m_desc(desc) {}

    Desc get_desc() const { return m_desc; }

private:
    Desc m_desc;
};



