#pragma once

#include "foxglove/resources/handle.h"
#include "foxglove/resources/fg_resource_types.h"
#include "foxglove/resources/frame_context.h"

#include "foxglove/core/types.h"

#include "foxglove/renderer/util.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <variant>

class Pass;
class PassBuilder;
class FrameGraph;

// usage and access refer to the resource's expected usage in this pass
// the resource's internal variables refer to its current state
// these are used to construct transitioninfo structs
struct BufferBinding {
    FGBufferHandle handle;
    BufferUsage usage;
    ResourceAccess access;
};

struct TextureBinding {
    FGTextureHandle handle;
    TextureUsage usage;
    ResourceAccess access;
};


enum class PassType {
    Graphics,
    Compute,
    Transfer,
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

class PassDesc {
public:
    void execute(PassContext ctx) const {
        m_execute_fn(ctx);
    }

    const PassType get_type() const { return m_type; }
    const std::vector<BufferBinding>& get_buffers() const { return m_buffers; }
    const std::vector<TextureBinding>& get_textures() const { return m_textures; }
    
private:
    friend PassBuilder;

    PassDesc(const std::string& name, PassType type,
            std::vector<BufferBinding> buffers,
            std::vector<TextureBinding> textures,
            std::function<void(PassContext)> execute_fn) :
        m_name(name),
        m_type(type),
        m_buffers(buffers),
        m_textures(textures),
        m_execute_fn(execute_fn) {}


    const std::string m_name;
    const PassType m_type;
    
    const std::vector<BufferBinding> m_buffers;
    const std::vector<TextureBinding> m_textures;
    
    // TODO: CONVERT TO FRAMECONTEXT
    const std::function<void(PassContext)> m_execute_fn;
};

// TODO: ADD PROLOGUE AND EPILOGUE PASSES
// TODO: ONLY ADD CONSUMERS IF THEYRE CROSSPIPELINE
class Pass {
public:
    Pass(PassDesc desc) : m_desc(desc) {}

    const PassDesc& get_desc() const { return m_desc; }
    const PassType get_type() const { return m_desc.get_type(); }
    const std::vector<BufferBinding>& get_buffers() const { return m_desc.get_buffers(); }
    const std::vector<TextureBinding>& get_textures() const { return m_desc.get_textures(); }

    void execute(PassContext ctx) { m_desc.execute(ctx); }
    
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

private:
    const PassDesc m_desc;

    bool m_culled = false;
    std::vector<Pass*> m_producers;

    std::vector<BufferTransitionInfo> m_buffer_transitions;
    std::vector<TextureTransitionInfo> m_texture_transitions;
    
    // TODO: ABSTRACT OUT VULKAN
    std::vector<VkBufferMemoryBarrier2> m_vk_buffer_barriers;
    std::vector<VkImageMemoryBarrier2> m_vk_image_barriers;
};


class PassBuilder {
public:
    PassBuilder(FrameGraph* fg, const std::string& name, PassType type) : 
        m_fg(fg), m_name(name), m_type(type) {}

    PassBuilder& bind_buffer(FGBufferHandle handle,
            BufferUsage usage, ResourceAccess access);
    PassBuilder& bind_texture(FGTextureHandle handle, 
            TextureUsage usage, ResourceAccess access);
    
    PassBuilder& present(FGTextureHandle handle);

    PassBuilder& clear_color(FGTextureHandle handle, Color color);
    PassBuilder& execute(std::function<void(PassContext)> execute_fn);

    Pass& build();    
private:
    FrameGraph* m_fg;
    std::string m_name;
    PassType m_type;
    
    // TODO: CHANGE TO DECLARATIONS
    std::vector<BufferBinding> m_buffers;
    std::vector<TextureBinding> m_textures;

    std::function<void(PassContext)> m_execute_fn;
};

