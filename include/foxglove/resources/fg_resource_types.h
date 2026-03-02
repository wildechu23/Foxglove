#pragma once

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
    StorageImage
};

enum class BufferUsage {
    StorageBuffer,
    UniformBuffer,
    VertexBuffer,
    IndexBuffer
    // Accleration structure
};

class Pass;

// TODO: CONSIDER WRAPPER FRAGMENTATION OF GENERIC PHYSICAL RESOURCE
class FGResource {
public:
    ~FGResource() = default;

    std::string get_name() const { return m_name; }
    ResourceAccess get_access() const { return m_access; }
    
    void set_access(ResourceAccess r) { m_access = r; }
    bool is_transient() const { return m_transient; }

    Pass* get_last_writer() const { return m_last_writer; }
protected:
    std::string m_name;
    ResourceAccess m_access;

    bool m_transient = true;
    
    // TODO: WHEN CONSIDERING MULTIPLE PIPELINES, EXTEND THIS
    Pass* m_last_writer;
};

class FGBuffer : public FGResource {
public:
    FGBuffer(BufferDesc desc) : m_desc(desc) {}
    
    BufferDesc get_desc() const { return m_desc; }
    BufferHandle get_handle() const { return m_handle; }
    BufferUsage get_usage() const { return m_usage; }
    BufferResource* get_resource() const { return m_resource; }
    
    void set_usage(BufferUsage bu) { m_usage = bu; }
    void set_resource(BufferResource* r) { m_resource = r; }
private:
    BufferDesc m_desc;
    BufferHandle m_handle;
    BufferUsage m_usage;
    
    BufferResource* m_resource;
};

class FGTexture : public FGResource {
public:
    FGTexture(TextureDesc desc) : m_desc(desc) {}
    
    TextureDesc get_desc() const { return m_desc; }
    TextureHandle get_handle() const { return m_handle; }
    TextureUsage get_usage() const { return m_usage; }
    TextureResource* get_resource() const { return m_resource; }

    void set_usage(TextureUsage tu) { m_usage = tu; }
    void set_resource(TextureResource* r) { m_resource = r; }
private:
    TextureDesc m_desc;
    
    // eventually change last_writer to handle mip levels and move out of base class
    TextureHandle m_handle;
    TextureUsage m_usage;

    TextureResource* m_resource;
};
