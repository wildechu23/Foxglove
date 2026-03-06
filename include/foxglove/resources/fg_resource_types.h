#pragma once

#include "foxglove/resources/handle_registry.h"

#include <string>

enum ResourceAccess {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = Read | Write
};

enum class TextureUsage {
    ColorAttachment,
    DepthAttachment,
    InputAttachment,
    StorageImage,
    TransferSrc,
    TransferDst
};

enum class BufferUsage {
    StorageBuffer,
    UniformBuffer,
    VertexBuffer,
    IndexBuffer,
    TransferSrc,
    TransferDst
    // Accleration structure
};

class Pass;

// TODO: CONSIDER WRAPPER FRAGMENTATION OF GENERIC PHYSICAL RESOURCE
class FGResource {
public:
    FGResource(const std::string& name) : m_name(name) {}
    ~FGResource() = default;

    std::string get_name() const { return m_name; }
    ResourceAccess get_access() const { return m_access; }
    
    void set_access(ResourceAccess r) { m_access = r; }
    bool is_transient() const { return m_transient; }
    
    bool collected() const { return m_collected; }
    void collect() { m_collected = true; }

    Pass* get_last_writer() const { return m_last_writer; }
    void set_last_writer(Pass* pass) { m_last_writer = pass; }
protected:
    std::string m_name;
    ResourceAccess m_access;

    bool m_transient = true;
    bool m_collected = false;
    
    // TODO: WHEN CONSIDERING MULTIPLE PIPELINES, EXTEND THIS
    Pass* m_last_writer;
};

// TODO: ADD NAMES
class FGBuffer : public FGResource {
public:
    FGBuffer(const std::string& name, BufferDesc desc) : 
        FGResource(name), m_desc(desc) {}
    
    BufferDesc get_desc() const { return m_desc; }
    FGBufferHandle get_handle() const { return m_handle; }
    BufferUsage get_usage() const { return m_usage; }
    BufferResource* get_resource() const { return m_resource; }
    
    void set_usage(BufferUsage bu) { m_usage = bu; }
    void set_resource(BufferResource* r) { m_resource = r; }
private:
    BufferDesc m_desc;
    FGBufferHandle m_handle;
    BufferUsage m_usage;
    
    BufferResource* m_resource;

    friend FGBufferRegistry;
};

class FGTexture : public FGResource {
public:
    FGTexture(const std::string& name, TextureDesc desc) : 
        FGResource(name), m_desc(desc) {}
    FGTexture(const std::string& name, TextureDesc desc, TextureResource* resource) : 
        FGResource(name), m_desc(desc), m_resource(resource) {
        m_transient = false;
    }
    
    TextureDesc get_desc() const { return m_desc; }
    FGTextureHandle get_handle() const { return m_handle; }
    TextureUsage get_usage() const { return m_usage; }
    TextureResource* get_resource() const { return m_resource; }

    void set_usage(TextureUsage tu) { m_usage = tu; }
    void set_resource(TextureResource* r) { m_resource = r; }
private:
    TextureDesc m_desc;
    
    // eventually change last_writer to handle mip levels and move out of base class
    FGTextureHandle m_handle;
    TextureUsage m_usage;

    TextureResource* m_resource;

    friend FGTextureRegistry;
};
