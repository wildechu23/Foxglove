#pragma once

#include "foxglove/core/types.h"

#include <optional>

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

struct RenderAttachment {
    FGTextureHandle handle;
    LoadOp load_op;
    StoreOp store_op;
    std::optional<Color> clear_color;
};

struct GraphicsAttachments {
    std::vector<RenderAttachment> color_attachments;
    RenderAttachment depth_attachment;
    RenderAttachment stencil_attachment;

    bool is_combined() const {
        return depth_attachment.handle == stencil_attachment.handle;
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
