#pragma once

#include "foxglove/framegraph/pass.h"
#include "foxglove/framegraph/pass_context.h"

#include "foxglove/core/types.h"

#include <optional>


class PassBuilder {
public:
    PassBuilder(FrameGraph* fg, const std::string& name, PassType type);
    
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
            std::optional<float> clear_value = std::nullopt);

    PassBuilder& bind_stencil_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<uint32_t> clear_value = std::nullopt);


    PassBuilder& bind_depth_stencil_attachment(FGTextureHandle handle,
            LoadOp load_op, StoreOp store_op,
            std::optional<float> depth_clear_value = std::nullopt,
            std::optional<uint32_t> stencil_clear_value = std::nullopt);

    PassBuilder& set_render_area(VkRect2D rect) { 
        m_render_area = rect;
        return *this;
    }

    PassBuilder& set_layer_count(uint32_t count) {
        m_layer_count = count;
        return *this;
    }
    
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
    std::vector<ColorAttachment> m_color_attachments;
    std::optional<DepthAttachment> m_depth_attachment;
    std::optional<StencilAttachment> m_stencil_attachment;

    VkRect2D m_render_area;
    uint32_t m_layer_count = 1;

    std::function<void(PassContext&)> m_execute_fn;
};

