#pragma once

#include "foxglove/resources/handle.h"
#include "foxglove/resources/fg_resource_types.h"
#include "foxglove/resources/attachments.h"

#include "foxglove/renderer/util.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <variant>

class PassContext;

class Pass;
class PassBuilder;
class FrameGraph;

struct ResourceBinding {
    uint32_t set;
    uint32_t binding;
    uint32_t array_idx = 0;
};

template<>
struct std::hash<ResourceBinding> {
    std::size_t operator()(const ResourceBinding& key) const {
        std::size_t h1 = std::hash<uint32_t>()(key.set);
        std::size_t h2 = std::hash<uint32_t>()(key.binding);
        std::size_t h3 = std::hash<uint32_t>()(key.array_idx);

        // Simple but effective combination
        return ((h1 ^ (h2 << 1)) ^ (h3 << 2));
    }
};

// usage and access refer to the resource's expected usage in this pass
// the resource's internal variables refer to its current state
// these are used to construct transitioninfo structs
struct BufferBinding {
    FGBufferHandle handle;
    BufferUsage usage;
    ResourceAccess access;
    uint32_t binding;
};

struct TextureBinding {
    FGTextureHandle handle;
    TextureUsage usage;
    ResourceAccess access;
    uint32_t binding;
};


enum class PassType {
    Graphics,
    Compute,
    Transfer,
    Clear,
    Present
};

struct BufferTransitionInfo {
    FGBufferHandle handle;
    
    BufferUsage src_usage;
    ResourceAccess src_access;
    
    BufferUsage dst_usage;
    ResourceAccess dst_access;
};

struct TextureTransitionInfo {
    FGTextureHandle handle;
    
    TextureUsage src_usage;
    ResourceAccess src_access;
    
    TextureUsage dst_usage;
    ResourceAccess dst_access;
};

struct BindingGroup {
    uint32_t set;
    std::vector<BufferBinding> buffers;
    std::vector<TextureBinding> textures;
};

class PassDesc {
public:
    void execute(PassContext& ctx) const {
        m_execute_fn(ctx);
    }

    const PassType get_type() const { return m_type; }

    const std::vector<BindingGroup>& get_bind_groups() const { return m_bind_groups; }
    
private:
    friend PassBuilder;

    PassDesc(const std::string& name, PassType type,
            std::vector<BindingGroup> bind_groups,
            std::function<void(PassContext&)> execute_fn) :
        m_name(name),
        m_type(type),
        m_bind_groups(bind_groups),
        m_execute_fn(execute_fn) {}


    const std::string m_name;
    const PassType m_type;
    
    const std::vector<BindingGroup> m_bind_groups;
    
    // TODO: CONVERT TO FRAMECONTEXT
    const std::function<void(PassContext&)> m_execute_fn;
};

// TODO: ADD PROLOGUE AND EPILOGUE PASSES
// TODO: ONLY ADD CONSUMERS IF THEYRE CROSSPIPELINE
class Pass {
public:
    Pass(PassDesc desc) : m_desc(desc) {}

    const PassDesc& get_desc() const { return m_desc; }
    const PassType get_type() const { return m_desc.get_type(); }
    //const std::vector<BufferBinding>& get_buffers() const { return m_desc.get_buffers(); }
    //const std::vector<TextureBinding>& get_textures() const { return m_desc.get_textures(); }

    void enumerate_buffers(std::function<void(const BufferBinding&)> fn) const {
        for(const BindingGroup& g : m_desc.get_bind_groups()) {
            for(const BufferBinding& bb : g.buffers) fn(bb);
        }
    }

    void enumerate_textures(std::function<void(const TextureBinding&)> fn) const {
        for(const BindingGroup& g : m_desc.get_bind_groups()) {
            for(const TextureBinding& tb : g.textures) fn(tb);
        }
    }
    
    const std::vector<BindingGroup>& get_bind_groups() const {
        return m_desc.get_bind_groups();
    }

    void execute(PassContext& ctx) { m_desc.execute(ctx); }

    std::vector<VkWriteDescriptorSet> build_writes();
    
    bool is_culled() { return m_culled; }
    std::vector<Pass*>& get_producers() { return m_producers; }
   
    std::vector<BufferTransitionInfo>& get_buffer_transitions() { 
        return m_buffer_transitions; 
    }
    std::vector<TextureTransitionInfo>& get_texture_transitions() { 
        return m_texture_transitions; 
    }

    void add_buffer_transition(BufferTransitionInfo bti) { 
        m_buffer_transitions.push_back(bti); 
    }

    void add_texture_transition(TextureTransitionInfo tti) { 
        m_texture_transitions.push_back(tti); 
    }

    std::vector<VkBufferMemoryBarrier2>& get_vk_buffer_barriers() { 
        return m_vk_buffer_barriers; 
    }
    std::vector<VkImageMemoryBarrier2>& get_vk_image_barriers() { 
        return m_vk_image_barriers; 
    }

    void add_vk_buffer_barrier(VkBufferMemoryBarrier2 bmb) {
        m_vk_buffer_barriers.push_back(bmb);
    }

    void add_vk_image_barrier(VkImageMemoryBarrier2 imb) {
        m_vk_image_barriers.push_back(imb);
    }

protected:
    const PassDesc m_desc;

    bool m_culled = false;
    std::vector<Pass*> m_producers;

    std::vector<BufferTransitionInfo> m_buffer_transitions;
    std::vector<TextureTransitionInfo> m_texture_transitions;
    
    // TODO: ABSTRACT OUT VULKAN
    std::vector<VkBufferMemoryBarrier2> m_vk_buffer_barriers;
    std::vector<VkImageMemoryBarrier2> m_vk_image_barriers;
};

class ComputePass : public Pass {
public:
    ComputePass(PassDesc desc) : Pass(desc) {}
};

class GraphicsPass : public Pass {
public:
    GraphicsPass(PassDesc desc, GraphicsPassInfo info) : 
        Pass(desc), m_info(info) {}

    const GraphicsPassInfo& get_info () const { return m_info; }

    void enumerate_textures(std::function<void(const TextureBinding&)> fn) const {
        // Descriptors
        for(const BindingGroup& g : m_desc.get_bind_groups()) {
            for(const TextureBinding& tb : g.textures) fn(tb);
        }
            
        // Attachments
        for(const ColorAttachment& ra : m_info.color_attachments) {
            fn({ ra.handle, TextureUsage::ColorAttachment, ResourceAccess::Write });
        }

        if(m_info.is_combined()) {
            FGTextureHandle handle = m_info.depth_attachment.value().handle;
            fn({ handle, TextureUsage::DepthStencilAttachment,
                    ResourceAccess::ReadWrite });
        } else {
            if(m_info.has_depth()) {
                FGTextureHandle handle = m_info.depth_attachment
                    .value().handle;
                
                fn({ handle, TextureUsage::DepthAttachment, 
                        ResourceAccess::ReadWrite });
            }
            if(m_info.has_stencil()) {
                FGTextureHandle handle = m_info.stencil_attachment
                    .value().handle;
                
                fn({ handle, TextureUsage::StencilAttachment, 
                        ResourceAccess::ReadWrite });
            }
        }
    }
private:
    GraphicsPassInfo m_info;

    friend class FrameGraph;

    bool b_attachments = false;

    std::vector<VkRenderingAttachmentInfo> m_color_attachment_info;
    VkRenderingAttachmentInfo m_depth_attachment_info;
    VkRenderingAttachmentInfo m_stencil_attachment_info;

    VkRenderingInfo m_rendering_info;
};

