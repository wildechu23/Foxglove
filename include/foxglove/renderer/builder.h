#pragma once

#include "foxglove/renderer/pass.h"
#include "foxglove/resources/pass_context.h"

#include "foxglove/core/types.h"

#include <optional>


class PassBuilder {
public:
    PassBuilder(FrameGraph* fg, const std::string& name, PassType type) : 
        m_fg(fg), m_name(name), m_type(type) {}
    
    // Descriptor binding
    PassBuilder& bind_buffer(FGBufferHandle handle,
            BufferUsage usage, ResourceAccess access,
            uint32_t set, uint32_t binding);
    PassBuilder& bind_texture(FGTextureHandle handle, 
            TextureUsage usage, ResourceAccess access,
            uint32_t set, uint32_t binding);
    
    // Render bindings
    PassBuilder& bind_color_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<Color> clear_value = std::nullopt);

    PassBuilder& bind_depth_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<Color> clear_value = std::nullopt);

    PassBuilder& bind_stencil_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<Color> clear_value = std::nullopt);

    PassBuilder& bind_depth_stencil_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<Color> clear_value = std::nullopt);
    
    PassBuilder& present(FGTextureHandle handle);

    PassBuilder& clear_color(FGTextureHandle handle, Color color);
    PassBuilder& execute(std::function<void(PassContext&)> execute_fn);

    Pass* build();    
private:
    void check_init(uint32_t set);

    FrameGraph* m_fg;
    std::string m_name;
    PassType m_type;
    
    // TODO: ADD IN THE SYSTEM USED ELSEWHERE
    // ITS IN SHADER.H AS DESCRIPTORSETLAYOUT.ADD_BINDING()
    std::unordered_map<uint32_t, BindingGroup> m_bind_groups;

    // Graphics
    std::vector<RenderAttachment> m_color_attachments;
    RenderAttachment m_depth_attachment;
    RenderAttachment m_stencil_attachment;

    std::function<void(PassContext&)> m_execute_fn;
};

