#include "foxglove/resources/pipeline/pipeline_manager.h"
#include "foxglove/core/types.h"

void GraphicsConfigBuilder::clear() {
    // clear all of the structs we need back to 0 with their correct stype
    m_inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    m_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    m_colorBlendAttachment = {};
    m_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    m_depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    m_renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    m_shader_set = {};
}


GraphicsPipelineConfig GraphicsConfigBuilder::build() {
    return GraphicsPipelineConfig{
        .shader_set = m_shader_set,
        .input_assembly = m_inputAssembly,
        .rasterizer = m_rasterizer,
        .color_blend_attachment = m_colorBlendAttachment,
        .multisampling = m_multisampling,
        .depth_stencil = m_depthStencil,
        .render_info = m_renderInfo,
        .color_attachment_format = m_colorAttachmentformat
    };
}

void GraphicsConfigBuilder::set_shaders(GraphicsShaderSet shaders) {
    m_shader_set = shaders;
}

void GraphicsConfigBuilder::set_input_topology(VkPrimitiveTopology topology) {
   m_inputAssembly.topology = topology;
    // we are not going to use primitive restart on the entire tutorial so leave
    // it on false
   m_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void GraphicsConfigBuilder::set_polygon_mode(VkPolygonMode mode) {
   m_rasterizer.polygonMode = mode;
   m_rasterizer.lineWidth = 1.f;
}

void GraphicsConfigBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
   m_rasterizer.cullMode = cullMode;
   m_rasterizer.frontFace = frontFace;
}

void GraphicsConfigBuilder::set_multisampling_none() {
   m_multisampling.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
   m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   m_multisampling.minSampleShading = 1.0f;
   m_multisampling.pSampleMask = nullptr;
    // no alpha to coverage either
   m_multisampling.alphaToCoverageEnable = VK_FALSE;
   m_multisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsConfigBuilder::disable_blending() {
    // default write mask
   m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // no blending
   m_colorBlendAttachment.blendEnable = VK_FALSE;
}

void GraphicsConfigBuilder::set_color_attachment_format(VkFormat format) {
   m_colorAttachmentformat = format;
// connect the format to the renderInfo  structure
   m_renderInfo.colorAttachmentCount = 1;
   m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentformat;
}

void GraphicsConfigBuilder::set_depth_format(VkFormat format) {
   m_renderInfo.depthAttachmentFormat = format;
}

void GraphicsConfigBuilder::disable_depthtest() {
   m_depthStencil.depthTestEnable = VK_FALSE;
   m_depthStencil.depthWriteEnable = VK_FALSE;
   m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
   m_depthStencil.depthBoundsTestEnable = VK_FALSE;
   m_depthStencil.stencilTestEnable = VK_FALSE;
   m_depthStencil.front = {};
   m_depthStencil.back = {};
   m_depthStencil.minDepthBounds = 0.f;
   m_depthStencil.maxDepthBounds = 1.f;
}


void GraphicsConfigBuilder::enable_depthtest(bool depthWriteEnable, 
        VkCompareOp op) {
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = depthWriteEnable;
    m_depthStencil.depthCompareOp = op;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    m_depthStencil.front = {};
    m_depthStencil.back = {};
    m_depthStencil.minDepthBounds = 0.f;
    m_depthStencil.maxDepthBounds = 1.f;
}

//
// PipelineManager
//

void PipelineManager::init(VkDevice device) {
    m_device = device;

    VkPipelineCacheCreateInfo cache_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0, // check if needed for initial data
    };

    if(vkCreatePipelineCache(m_device, &cache_info, nullptr, 
            &m_vk_pipeline_cache) != VK_SUCCESS) {
        printf("pipeline cache fucked up\n");
    }
}

void PipelineManager::cleanup() {
    vkDestroyPipelineCache(m_device, m_vk_pipeline_cache, nullptr);

    for (const auto& [key, pipeline] : m_compute_pipeline_cache) {
        pipeline->cleanup();
    }

    for (const auto& [key, pipeline] : m_graphics_pipeline_cache) {
        pipeline->cleanup();
    }
}

void PipelineManager::create_descriptor_set_layouts(Pipeline* pipeline, 
        std::vector<DescriptorSetLayout>& layouts) {
    pipeline->m_descriptor_layouts.reserve(layouts.size());
    for(size_t i = 0; i < layouts.size(); ++i) {
        DescriptorSetLayout& layout = layouts[i];

        VkDescriptorSetLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(layout.bindings.size()),
            .pBindings = layout.bindings.data()
        };    

        VkDescriptorSetLayout vk_layout;

        // TODO: MAKE IT MATCH SETNUMBER?
        vkCreateDescriptorSetLayout(m_device, &create_info, nullptr, 
                &vk_layout);
        pipeline->m_descriptor_layouts.push_back(vk_layout);
    }
}

void PipelineManager::create_push_constant_ranges(Pipeline* pipeline, 
        std::vector<PushConstantInfo>& pc_infos) {
    pipeline->m_push_constant_ranges.reserve(pc_infos.size());
    for(size_t i = 0; i < pc_infos.size(); ++i) {
        PushConstantInfo& info = pc_infos[i];

        VkPushConstantRange range = {
            .stageFlags = info.stage,
            .offset = info.offset,
            .size = info.size
        };

        pipeline->m_push_constant_ranges.push_back(range);
    }
}

ComputePipeline* PipelineManager::get_compute_pipeline(ComputeShader* shader) {
    ComputePipelineDesc desc = { shader };
    uint64_t key = desc.create_key();

    auto itr = m_compute_pipeline_cache.find(key);
    if(itr != m_compute_pipeline_cache.end()) {
        return itr->second;
    }

    ComputePipeline* pipeline = new ComputePipeline(m_device, desc);

    // create descriptorsetlayouts
    std::vector<DescriptorSetLayout> layouts = 
        ShaderLibrary::reflect_layout(shader);
    std::vector<PushConstantInfo> infos = 
        ShaderLibrary::reflect_push_constants(shader);

    create_descriptor_set_layouts(pipeline, layouts);
    create_push_constant_ranges(pipeline, infos);

    std::vector<VkDescriptorSetLayout>& vk_layouts = 
        pipeline->m_descriptor_layouts;
    std::vector<VkPushConstantRange>& vk_ranges =
        pipeline->m_push_constant_ranges;

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = static_cast<uint32_t>(vk_layouts.size()),
        .pSetLayouts = vk_layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(vk_ranges.size()),
        .pPushConstantRanges = vk_ranges.data()
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
        .layout = pipeline->m_pipeline_layout
    };

    // TODO: consider bundling them i dunno
    vkCreateComputePipelines(m_device, m_vk_pipeline_cache, 1, 
            &compute_pipeline_info, nullptr, &pipeline->m_pipeline);
    
    // add to cache
    m_compute_pipeline_cache[key] = pipeline; 
    return pipeline;
}


void PipelineManager::create_graphics_desc(
        GraphicsPipelineConfig& config,
        GraphicsPipelineDesc* desc) {
    desc->m_color_blend_attachments.push_back(
            config.color_blend_attachment);
    
    desc->m_multisampling = config.multisampling;
    desc->m_input_assembly = config.input_assembly;
    desc->m_rasterizer = config.rasterizer;
    desc->m_depth_stencil = config.depth_stencil;

    desc->m_color_attachment_formats.push_back(
            config.color_attachment_format);
    desc->m_depth_attachment_format = config.render_info.depthAttachmentFormat;
}

void PipelineManager::build_graphics_pipeline(
        GraphicsPipeline* pipeline,
        GraphicsShaderSet& set ) {
    const GraphicsPipelineDesc& desc = pipeline->get_desc();

    // Handle shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	VkPipelineShaderStageCreateInfo vs_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = set.vs->get_module(),
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fs_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = set.fs->get_module(),
		.pName = "main"
	};

    shader_stages.push_back(vs_info);
    shader_stages.push_back(fs_info);
    
    // Build pipeline layout
    std::vector<VkDescriptorSetLayout>& layouts = 
        pipeline->m_descriptor_layouts;
    std::vector<VkPushConstantRange>& ranges =
        pipeline->m_push_constant_ranges;

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(ranges.size()),
        .pPushConstantRanges = ranges.data()
    };

    vkCreatePipelineLayout(m_device, &layout_info, nullptr, 
            &pipeline->m_pipeline_layout);
    
    // Build pipeline
    VkPipelineViewportStateCreateInfo viewportState = {
    	.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    	.pNext = nullptr,
		.viewportCount = 1,
    	.scissorCount = 1
	};
    
    VkPipelineRenderingCreateInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = desc.m_view_mask,
        .colorAttachmentCount = static_cast<uint32_t>(desc.m_color_attachment_formats.size()),
        .pColorAttachmentFormats = desc.m_color_attachment_formats.data(),
        .depthAttachmentFormat = desc.m_depth_attachment_format,
        .stencilAttachmentFormat = desc.m_stencil_attachment_format
    };

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {
    	.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    	.pNext = nullptr,
    	.logicOpEnable = VK_FALSE,
    	.logicOp = VK_LOGIC_OP_COPY,
    	.attachmentCount = static_cast<uint32_t>(desc.m_color_blend_attachments.size()),
    	.pAttachments = desc.m_color_blend_attachments.data()
	};

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { 
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0
	};
    
    // Dynamic states
	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = &state[0]
	};


	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &render_info,
		.stageCount = (uint32_t)shader_stages.size(),
		.pStages = shader_stages.data(),
		.pVertexInputState = &_vertexInputInfo,
		.pInputAssemblyState = &desc.m_input_assembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &desc.m_rasterizer,
		.pMultisampleState = &desc.m_multisampling,
		.pDepthStencilState = &desc.m_depth_stencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicInfo,
		.layout = pipeline->m_pipeline_layout
	};

	if(vkCreateGraphicsPipelines(m_device, m_vk_pipeline_cache, 1, &pipelineInfo, nullptr, &pipeline->m_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline");
	}
}


GraphicsPipeline* PipelineManager::get_graphics_pipeline(GraphicsPipelineConfig& config) {
    GraphicsPipelineDesc desc;
    create_graphics_desc(config, &desc);

    // check cache
    uint64_t key = desc.create_key();
    auto itr = m_graphics_pipeline_cache.find(key);
    if(itr != m_graphics_pipeline_cache.end()) {
        return itr->second;
    }

    // create it from scratch
    GraphicsPipeline* pipeline = new GraphicsPipeline(m_device, desc);

    // grab and merge?
    GraphicsShaderSet& set = config.shader_set;
    std::vector<DescriptorSetLayout> final_layouts;
    std::vector<PushConstantInfo> final_pc_infos;

    std::array<Shader*, 3> shaders = { set.vs, set.fs, set.gs };
    for(Shader* shader : shaders) {
        if(!shader) continue;

        std::vector<DescriptorSetLayout> layouts = 
            ShaderLibrary::reflect_layout(shader);

        for(DescriptorSetLayout& layout : layouts) {
            uint32_t set = layout.set_number;
            if(set >= final_layouts.size()) {
                final_layouts.resize(set + 1);
            }

            DescriptorSetLayout& new_l = final_layouts[set];
            for(auto& binding : layout.bindings) {
                new_l.add_binding(binding);
            }
        }

        std::vector<PushConstantInfo> infos = 
            ShaderLibrary::reflect_push_constants(shader);
        
        for(PushConstantInfo& info : infos) {
            bool merged = false;
            for(PushConstantInfo& old : final_pc_infos) {
                if(old.size == info.size && old.offset == info.offset) {
                    old.stage |= info.stage;
                    break;
                }
            }
            if(!merged) final_pc_infos.push_back(info);
        }
    }

    // erase empty sets?
    final_layouts.erase(
        std::remove_if(final_layouts.begin(), final_layouts.end(),
            [](const DescriptorSetLayout& layout) { 
                return layout.bindings.empty(); 
            }),
        final_layouts.end());
    
    // TODO: SORT HERE? SORT NECESSARY?
    std::sort(final_pc_infos.begin(), final_pc_infos.end(),
        [](const PushConstantInfo& a, const PushConstantInfo& b) {
            return a.offset < b.offset;
        });

    create_descriptor_set_layouts(pipeline, final_layouts);
    create_push_constant_ranges(pipeline, final_pc_infos);

    build_graphics_pipeline(pipeline, config.shader_set);

    // add to cache
    m_graphics_pipeline_cache[key] = pipeline; 
    return pipeline;
}
