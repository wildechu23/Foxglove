#pragma once

#include "foxglove/core/types.h"
#include "foxglove/framegraph/fg_resource.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <stdexcept>

// TODO: EXT
enum class LoadOp {
    Load,
    Clear,
    DontCare,
    None
};

enum class StoreOp {
    Store,
    DontCare,
    None
};

struct ColorAttachment {
    FGTextureHandle handle;
    LoadOp load_op;
    StoreOp store_op;
    std::optional<Color> clear_value;
};

struct DepthAttachment {
    FGTextureHandle handle;
    LoadOp load_op;
    StoreOp store_op;
    std::optional<float> clear_value;
};

struct StencilAttachment {
    FGTextureHandle handle;
    LoadOp load_op;
    StoreOp store_op;
    std::optional<uint32_t> clear_value;
};

struct GraphicsPassInfo {
    std::vector<ColorAttachment> color_attachments;
    std::optional<DepthAttachment> depth_attachment;
    std::optional<StencilAttachment> stencil_attachment;

    VkRect2D render_area;
    uint32_t layer_count = 1;

    bool has_depth() const { return depth_attachment.has_value(); }
    bool has_stencil() const { return stencil_attachment.has_value(); }

    bool is_combined() const {
        return has_depth() && has_stencil() && depth_attachment.value().handle 
            == stencil_attachment.value().handle;
    }
};

namespace util {
    inline VkAttachmentLoadOp to_vulkan(LoadOp op) {
        switch(op) {
            case LoadOp::Load:      return VK_ATTACHMENT_LOAD_OP_LOAD;
            case LoadOp::Clear:     return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case LoadOp::DontCare:  return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            case LoadOp::None:      return VK_ATTACHMENT_LOAD_OP_NONE; 
            default: throw std::runtime_error("Unknown LoadOp");
        }
    }

    inline VkAttachmentStoreOp to_vulkan(StoreOp op) {
        switch(op) {
            case StoreOp::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
            case StoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            case StoreOp::None:     return VK_ATTACHMENT_STORE_OP_NONE; 
            default: throw std::runtime_error("Unknown StoreOp");
        }
    }
}
