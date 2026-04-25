#pragma once

#include <glm/glm.hpp>

#include <cstdint>

struct Color {
    float r, g, b, a;
};

enum class ResourceType : uint8_t {
    Buffer = 1,
    Texture = 2
};


enum class JobType : uint8_t {
    Upload = 1
};



