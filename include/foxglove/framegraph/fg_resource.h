#pragma once

#include "foxglove/core/types.h"
#include "foxglove/resources/resource.h"
#include "foxglove/resources/desc.h"
#include "foxglove/resources/handle_registry.h"

#include <string>
#include <cassert>

using FGBufferHandle = TaggedTHandle<ResourceType::Buffer>;
using FGTextureHandle = TaggedTHandle<ResourceType::Texture>;

using FGBufferRegistry = LLHandleRegistry<FGBuffer, FGBufferHandle>;
using FGTextureRegistry = LLHandleRegistry<FGTexture, FGTextureHandle>;

enum ResourceAccess {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = Read | Write
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

enum class TextureUsage {
    ColorAttachment,
    DepthAttachment,
    StencilAttachment,
    DepthStencilAttachment,
    InputAttachment,
    StorageImage,
    TransferSrc,
    TransferDst
};

class Pass;

// TODO: CONSIDER WRAPPER FRAGMENTATION OF GENERIC PHYSICAL RESOURCE
class FGResource {
public:
    FGResource(const std::string& name, ResourceType type) : 
        m_name(name), m_type(type) {}
    FGResource(const std::string& name, ResourceType type, bool is_transient) : 
        m_name(name), m_type(type), m_transient(is_transient) {}

    ~FGResource() = default;

    std::string get_name() const { return m_name; }
    ResourceAccess get_access() const { return m_access; }
    ResourceType get_type() const { return m_type; }
    
    void set_access(ResourceAccess r) { m_access = r; }
    bool is_transient() const { return m_transient; }
    
    // for culling i think
    bool collected() const { return m_collected; }
    void collect() { m_collected = true; }

    Pass* get_last_writer() const { return m_last_writer; }
    void set_last_writer(Pass* pass) { m_last_writer = pass; }
protected:
    std::string m_name;
    ResourceAccess m_access;

    ResourceType m_type;
    
    // upgrade transient to enum if needed
    bool m_transient = true;
    bool m_collected = false;
    
    // TODO: WHEN CONSIDERING MULTIPLE PIPELINES, EXTEND THIS
    Pass* m_last_writer;
};

class FGBuffer : public FGResource {
public:
    FGBuffer(const std::string& name, BufferDesc desc) : 
        FGResource(name, ResourceType::Buffer), m_desc(desc) {}
    FGBuffer(const std::string& name, BufferHandle resource) : 
        FGResource(name, ResourceType::Buffer, false), 
        m_resource_handle(resource) {}
    
    BufferDesc get_desc() const { return m_desc; }
    FGBufferHandle get_handle() const { return m_handle; }
    BufferUsage get_usage() const { return m_usage; }
        
    BufferHandle    get_resource() const { return m_resource_handle; }
    BufferResource* get_resource_ptr() const { return m_resource_ptr; }
    
    void set_usage(BufferUsage bu) { m_usage = bu; }
    void set_resource(BufferHandle r) { m_resource_handle = r; }
    void set_resource_ptr(BufferResource* r) { m_resource_ptr = r; }
private:
    BufferDesc m_desc;
    FGBufferHandle m_handle;
    BufferUsage m_usage;
   
    BufferHandle    m_resource_handle;
    BufferResource* m_resource_ptr; // ONLY USE IF VALID

    friend FGBufferRegistry;
};

class FGTexture : public FGResource {
public:
    FGTexture(const std::string& name, TextureDesc desc) : 
        FGResource(name, ResourceType::Texture), m_desc(desc) {}
    FGTexture(const std::string& name, TextureDesc desc, TextureHandle resource) 
        : FGResource(name, ResourceType::Texture, false), 
        m_desc(desc), m_resource_handle(resource) {}


    TextureDesc get_desc() const { return m_desc; }
    FGTextureHandle get_handle() const { return m_handle; }
    TextureUsage get_usage() const { return m_usage; }

    TextureHandle    get_resource() const { return m_resource_handle; }
    TextureResource* get_resource_ptr() const { return m_resource_ptr; }

    void set_usage(TextureUsage tu) { m_usage = tu; }
    void set_resource(TextureHandle r) { m_resource_handle = r; }
    void set_resource_ptr(TextureResource* r) { m_resource_ptr = r; }
private:
    TextureDesc m_desc;
    
    // eventually change last_writer to handle mip levels and move out of base class
    FGTextureHandle m_handle;
    TextureUsage m_usage;

    TextureHandle    m_resource_handle;
    TextureResource* m_resource_ptr; // ONLY USE IF VALID

    friend FGTextureRegistry;
};
