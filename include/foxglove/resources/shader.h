#pragma once

#include "vulkan/vulkan.h"

#include <cstdint>
#include <string>
#include <vector>
#include <span>



// use if needed?
enum class ShaderStage : uint8_t {
    Vertex = 0,
    Pixel,
    Compute,
    Geometry
};

struct DescriptorSetLayout {
    uint32_t set_number;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class Shader {
public:
    virtual ~Shader() = default;

protected:
    Shader(const std::string& name, ShaderStage stage, 
            VkShaderModule shader_module, 
            std::span<uint32_t> code) : 
        m_name(name), m_stage(stage), m_module(shader_module),
        m_code(code) {}
 
    std::string m_name;
    ShaderStage m_stage;
    VkShaderModule m_module;
    std::span<uint32_t> m_code;
    

    friend class ShaderLibrary;
    friend class PipelineManager;
};

class ComputeShader : public Shader {
public:
    ComputeShader(const std::string& name, 
            VkShaderModule shader_module,
            std::span<uint32_t> code) : 
        Shader(name, ShaderStage::Compute, shader_module, code) {}
};

class GraphicsShader : public Shader {

};
