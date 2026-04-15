#pragma once

#include "foxglove/renderer/pass.h"
#include "foxglove/renderer/builder.h"

#include "foxglove/resources/handle_registry.h"
#include "foxglove/resources/resource_types.h"
#include "foxglove/resources/fg_resource_types.h"
#include "foxglove/resources/frame_context.h"

#include "foxglove/vulkan/vulkan_context.h"
#include "foxglove/vulkan/swapchain.h"
#include "foxglove/vulkan/descriptors.h"


#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <algorithm>

struct ResourceState {
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags access = 0;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
};

class FrameGraph {
public:
    void init(VulkanContext* ctx, Swapchain* swapchain);
    void reset();

    FGBufferHandle create_buffer(const std::string& name, BufferDesc desc);
    FGTextureHandle create_texture(const std::string& name, TextureDesc desc);
    FGTextureHandle register_external_texture(const std::string& name,
            TextureDesc desc, TextureResource* resource);

    PassBuilder create_pass(const std::string& name, PassType type);

    void compile();
    void execute(FrameContext& ctx);
private:
    void add_pass(std::unique_ptr<Pass> pass);
    
    void build_dependencies();
    void allocate_resources(FrameContext& fctx);

    //void collect_pass_bindings(DescriptorLayoutBuilder& builder, Pass* pass);
    //void collect_descriptors();

    void collect_pass_barriers();
    void compile_pass_barriers(FrameContext& fctx);

    VkRenderingInfo get_rendering_info(GraphicsPass* pass);

    std::vector<std::unique_ptr<Pass>> m_passes;

    FGBufferRegistry m_buffers;
    FGTextureRegistry m_textures;
    
    //DescriptorAllocator m_desc_allocator;

    VulkanContext* m_ctx;
    Swapchain* m_swapchain;

    // check if needs compilation
    bool m_dirty = false;

    friend class PassBuilder;
};
