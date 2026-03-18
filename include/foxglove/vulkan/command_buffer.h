#pragma once

#include "foxglove/resources/shader.h"

#include <cstdint>
#include <vector>
#include <filesystem>


class CommandBuffer {
public:
    explicit CommandBuffer(VkCommandBuffer cmd) : m_cmd(cmd) {}
    
    void bind_compute_shader(ComputeShader* shader);
    void dispatch(uint32_t x, uint32_t y, uint32_t z);
private:
    VkCommandBuffer m_cmd;

    friend class PassBuilder;
};

