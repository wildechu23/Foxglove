#pragma once

#include "shader.h"

#include "spirv_reflect.h"

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <span>

#include <iostream>
#include <vulkan/vulkan.h>

class ShaderLibrary {
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;

    void init(VkDevice device) {
        m_device = device;
    }

    void cleanup() {
        for (const auto& [key, shader] : m_shader_map) {
            shader->cleanup(m_device);
        }
    };

    ComputeShader* create_compute_shader(const std::filesystem::path& path) {
        return create_shader<ComputeShader>(path);
    }

    static std::vector<DescriptorSetLayout> reflect_layout(Shader* shader) {
        std::span<uint32_t> code = shader->m_code;

        SpvReflectShaderModule spvmodule;
        SpvReflectResult result = spvReflectCreateShaderModule(
                code.size() * sizeof(uint32_t), code.data(), 
                &spvmodule);

        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            std::cerr << "Error: Shader reflect failed" << std::endl;
            return {};
        }
            
        // count pass
        uint32_t count = 0;
        spvReflectEnumerateDescriptorSets(&spvmodule, &count, nullptr);
        std::cout << "count: " << count << std::endl;

        // data pass
        std::vector<SpvReflectDescriptorSet*> sets(count);
        result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());

        std::vector<DescriptorSetLayout> set_layouts;

        for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
            const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);

            DescriptorSetLayout layout{};

            layout.bindings.resize(refl_set.binding_count);
            for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
                const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);

                VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
                layout_binding.binding = refl_binding.binding;
                layout_binding.descriptorType = static_cast<VkDescriptorType>(
                        refl_binding.descriptor_type);
                layout_binding.descriptorCount = 1;
                for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
                    layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
                }
                layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(
                        spvmodule.shader_stage);
            }
            layout.set_number = refl_set.set;
            set_layouts.push_back(layout);
        }

        return set_layouts;
    }

private:
    template <typename ShaderT>
    ShaderT* create_shader(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "File not found" << std::endl;
            return nullptr;
        }

        size_t file_size = (size_t)file.tellg();
        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), file_size);
        file.close();

        VkShaderModuleCreateInfo create_info = { 
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .codeSize = buffer.size() * sizeof(uint32_t),
            .pCode = buffer.data()
        };

        VkShaderModule shader_module;
        if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
            return nullptr;
        }

        const std::string name = path.filename().string(); 
        ShaderT* out_shader = new ShaderT(name, shader_module, std::move(buffer));
        m_shader_map[counter++] = out_shader;

        return out_shader;
    }

    VkDevice m_device;
    // TODO: remove counter
    int counter = 0;
    std::unordered_map<uint64_t, Shader*> m_shader_map;
};
