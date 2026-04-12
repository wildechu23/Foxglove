#pragma once

#include "vulkan/vulkan.h"

#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include <algorithm>


// use if needed?
enum class ShaderStage : uint8_t {
    Vertex = 0,
    Mesh,
    Fragment,
    Compute,
    Geometry
};

struct DescriptorSetLayout {
    uint32_t set_number;
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(const VkDescriptorSetLayoutBinding& new_binding) {
        auto it = std::lower_bound(bindings.begin(), 
                bindings.end(), new_binding.binding,
            [](const VkDescriptorSetLayoutBinding& a, uint32_t binding) { 
                return a.binding < binding; 
            });
        
        if (it != bindings.end() && it->binding == new_binding.binding) {
            it->stageFlags |= new_binding.stageFlags;
            // TODO: check for mismatch here?
        } else {
            bindings.insert(it, new_binding);
        }
    }
};

class Shader {
public:
    virtual ~Shader() = default;

    VkShaderModule get_module() { return m_module; }

    void cleanup(VkDevice device) {
        vkDestroyShaderModule(device, m_module, nullptr);
    }
protected:
    Shader(const std::string& name, 
            VkShaderModule shader_module, 
            std::vector<uint32_t> code) : 
        m_name(name), m_module(shader_module),
        m_code(std::move(code)) {}
    
    std::string m_name;
    ShaderStage m_stage;
    VkShaderModule m_module;
    std::vector<uint32_t> m_code;

    friend class ShaderLibrary;
    friend class PipelineManager;
};



template<ShaderStage ShaderT>
class ShaderBase : public Shader {
public:
    ShaderBase(const std::string& name,
            VkShaderModule shader_module,
            std::vector<uint32_t> code) :
        Shader(name, shader_module, code) {}

    static inline constexpr ShaderStage stage = ShaderT;
};

typedef ShaderBase<ShaderStage::Vertex> VertexShader;
typedef ShaderBase<ShaderStage::Mesh> MeshShader;
typedef ShaderBase<ShaderStage::Fragment> FragmentShader;
typedef ShaderBase<ShaderStage::Compute> ComputeShader;
typedef ShaderBase<ShaderStage::Geometry> GeometryShader;
