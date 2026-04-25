#pragma once

#include "foxglove/resources/pipeline/pipeline.h"
#include "foxglove/resources/shader/shader_library.h"

#include "spirv_reflect.h"

#include <unordered_map>
#include <vulkan/vulkan.h>


// TODO: FIX CAMELCASE
class GraphicsConfigBuilder {
public:
    GraphicsConfigBuilder(){ clear(); }

    void clear();
    
    GraphicsPipelineConfig build();
    void set_shaders(GraphicsShaderSet shaders);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_multisampling_none();

    void disable_blending();
    void set_color_attachment_format(VkFormat format);
    void set_depth_format(VkFormat format);
    void disable_depthtest();
    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
private:
    GraphicsShaderSet                       m_shader_set;

    VkPipelineInputAssemblyStateCreateInfo  m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo  m_rasterizer;
    VkPipelineColorBlendAttachmentState     m_colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo    m_multisampling;
    VkPipelineDepthStencilStateCreateInfo   m_depthStencil;
    VkPipelineRenderingCreateInfo           m_renderInfo;
    VkFormat                                m_colorAttachmentformat;
};


class PipelineManager {
public:
    void init(VkDevice device);
    void cleanup();
    
    ComputePipeline* get_compute_pipeline(ComputeShader* shader); 
    GraphicsPipeline* get_graphics_pipeline(GraphicsPipelineConfig& config);

private:
    VkDevice m_device;

    void create_descriptor_set_layouts(Pipeline* pipeline, 
            std::vector<DescriptorSetLayout>& layouts);
    void create_push_constant_ranges(Pipeline* pipeline, 
            std::vector<PushConstantInfo>& pc_infos);

    // graphics stuff
    void create_graphics_desc(GraphicsPipelineConfig& config,
            GraphicsPipelineDesc* desc);
    void build_graphics_pipeline(GraphicsPipeline* pipeline, 
            GraphicsShaderSet& set);


    // pipeline caches, will need to be thread safe eventually
    // unreal stores a local cache per thread and global cache 
    
    std::unordered_map<uint64_t, ComputePipeline*> m_compute_pipeline_cache;
    std::unordered_map<uint64_t, GraphicsPipeline*> m_graphics_pipeline_cache;

    VkPipelineCache m_vk_pipeline_cache;
};
