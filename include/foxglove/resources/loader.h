#pragma once

#include "foxglove/core/types.h"
#include "foxglove/core/handle.h"
#include "foxglove/resources/resource.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <vector>
#include <optional>
#include <filesystem>

namespace fs = std::filesystem;

class ResourceManager;
class UploadManager;

// bindless vertex
struct Vertex {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

struct GeoSurface {
    uint32_t start_index;
    uint32_t count;
};

struct MeshData {
    std::string name;
    std::vector<GeoSurface> surfaces;
    
    BufferHandle index_buffer;
    BufferHandle vertex_buffer;
};

struct LoadedGLTF {
    std::unordered_map<std::string, std::shared_ptr<MeshData>> meshes;
    std::unordered_map<std::string, TextureHandle> images;
    std::vector<SamplerHandle> samplers;

    BufferHandle material_data_buffer;
};


class Loader {
public:
    Loader(ResourceManager& rm, UploadManager& um) 
        : m_rm(rm), m_um(um) {}
    ~Loader() = default;

    std::optional<std::shared_ptr<LoadedGLTF>> load_gltf_meshes(
            fs::path file_path);
    std::optional<TextureHandle> load_image(fastgltf::Asset& asset, 
            fastgltf::Image& image);
private:
    ResourceManager& m_rm;
    UploadManager& m_um;
};
