#pragma once

#include "foxglove/core/types.h"
#include "foxglove/core/handle.h"
#include "foxglove/resources/resource.h"

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


class Loader {
public:
    Loader(ResourceManager& rm, UploadManager& um) 
        : m_rm(rm), m_um(um) {}
    ~Loader() = default;

    std::optional<std::vector<std::shared_ptr<MeshData>>> load_gltf_meshes(
            fs::path file_path);
private:
    ResourceManager& m_rm;
    UploadManager& m_um;
};
