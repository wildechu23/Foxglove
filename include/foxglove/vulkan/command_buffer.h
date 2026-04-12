#pragma once
/*
#include "foxglove/resources/shader.h"
#include "foxglove/resources/pipeline.h"
#include "foxglove/resources/fg_resource_types.h"

#include "foxglove/renderer/util.h"

#include <cstdint>
#include <vector>
#include <filesystem>
#include <variant>
#include <unordered_map>

struct ShaderParameter {
    uint32_t set;
    uint32_t binding;
    FGResource* resource;
};

// todo: flat array??
typedef std::vector<std::vector<std::vector<FGResource*>>> ShaderParameters;

using DescriptorWriteInfo = std::variant<
        std::vector<VkDescriptorBufferInfo>,
        std::vector<VkDescriptorImageInfo>>;


class CommandContext {
public:
    CommandContext(VkCommandBuffer cmd, VkDevice device, Pass* pass) : 
        m_cmd(cmd), m_device(device), m_pass(pass) {}
   
    void bind_compute_pipeline(ComputePipeline* pipeline);
    void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);
private:
    void get_params();

    VkCommandBuffer m_cmd;
    VkDevice m_device;
    Pass* m_pass;
    
    ComputePipeline* m_curr_pipeline;

    ShaderParameters m_curr_params;

    std::unordered_map<ResourceBinding, FGResource*>

    friend class PassBuilder;
};
*/
