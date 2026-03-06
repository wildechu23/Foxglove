#pragma once

#include "foxglove/resources/resource_types.h"

class HandleBase {
public:
    uint64_t get_data() const { return m_data; }

    static constexpr uint32_t INVALID = UINT32_MAX;
    bool is_valid() const { return m_data != INVALID; }
protected:
    constexpr HandleBase() : m_data(INVALID) {}
    constexpr HandleBase(uint64_t data) : m_data(data) {}
    uint64_t m_data;
};


// GENERATION HANDLE, HANDLES CAN ALSO NOT NEED IT
// TODO: consider swapping to IHandle and moving this to taggedhandle
class Handle : public HandleBase {
public:
    Handle() = default; 
    Handle(const Handle&) = default;
    Handle& operator=(const Handle&) = default;

    // type:    8 bits
    // index:   24 bits
    // gen:     8 bits
    static constexpr uint64_t TYPE_SHIFT = 32;
    static constexpr uint64_t INDEX_SHIFT = 8;
    static constexpr uint64_t GEN_SHIFT = 0;

    static constexpr uint64_t TYPE_MASK = 0xFFULL << TYPE_SHIFT;
    static constexpr uint64_t INDEX_MASK = 0xFFFFFFULL << INDEX_SHIFT;
    static constexpr uint64_t GEN_MASK = 0xFFULL;
    
    //static Handle invalid() { return Handle(INVALID); }

    bool operator==(const Handle& other) const {
        return m_data == other.m_data;
    }

    uint64_t get_type_id() const { return (m_data & TYPE_MASK) >> TYPE_SHIFT ; }
    uint64_t get_index() const { return (m_data & INDEX_MASK) >> INDEX_SHIFT ; }
    uint64_t get_gen() const { return (m_data & GEN_MASK) >> GEN_SHIFT; }
protected:
    Handle(uint64_t data) : HandleBase(data) {}
    Handle(uint64_t type_id, uint64_t index, uint64_t gen)
        : HandleBase((type_id << TYPE_SHIFT) |
                (index << INDEX_SHIFT) |
                (gen << GEN_SHIFT)) {}
};

class THandle : public HandleBase {
public:
    THandle() = default; 
    THandle(const THandle&) = default;
    THandle& operator=(const THandle&) = default;

    // type:    8 bits
    // index:   32 bits
    static constexpr uint64_t TYPE_SHIFT = 32;
    static constexpr uint64_t INDEX_SHIFT = 0;

    static constexpr uint64_t TYPE_MASK = 0xFFULL << TYPE_SHIFT;
    static constexpr uint64_t INDEX_MASK = 0xFFFFFFULL << INDEX_SHIFT;

    bool operator==(const THandle& other) const {
        return m_data == other.m_data;
    }

    uint64_t get_type_id() const { return (m_data & TYPE_MASK) >> TYPE_SHIFT ; }
    uint64_t get_index() const { return (m_data & INDEX_MASK) >> INDEX_SHIFT ; }
protected:
    THandle(uint64_t data) : HandleBase(data) {}
    constexpr THandle(uint64_t type_id, uint64_t index)
        : HandleBase((type_id << TYPE_SHIFT) | 
                (index << INDEX_SHIFT)) {}
};

template<typename Tag, TypeID TypeIDValue>
class TaggedHandle : public Handle {
public:
    static constexpr uint8_t TYPE_ID = static_cast<uint8_t>(TypeIDValue);
    static constexpr uint8_t type_id() { return TYPE_ID; }
    
    TaggedHandle() : Handle(INVALID) {}

    static TaggedHandle invalid() { 
        return TaggedHandle(INVALID); 
    }

    constexpr TaggedHandle(uint32_t index, uint32_t gen) :
        Handle(TYPE_ID, index, gen) {}

    TaggedHandle(const Handle& h) : Handle(h.m_data) {}
    
    TaggedHandle(const TaggedHandle&) = default;
    TaggedHandle& operator=(const TaggedHandle&) = default;
};

template<typename Tag, TypeID TypeIDValue>
class TaggedTHandle : public THandle {
public:
    static constexpr uint8_t TYPE_ID = static_cast<uint8_t>(TypeIDValue);
    static constexpr uint8_t type_id() { return TYPE_ID; }
    
    TaggedTHandle() : THandle(INVALID) {}

    static TaggedTHandle invalid() { 
        return TaggedTHandle(INVALID); 
    }

    constexpr TaggedTHandle(uint32_t index) :
        THandle(TYPE_ID, index) {}

    TaggedTHandle(const THandle& h) : 
        THandle(h.m_data) {}
    
    TaggedTHandle(const TaggedTHandle&) = default;
    TaggedTHandle& operator=(const TaggedTHandle&) = default;
};


// Tag types
struct BufferTag {};
struct TextureTag {};

// Typedefs
using BufferHandle = TaggedHandle<BufferTag, TypeID::Buffer>;
using TextureHandle = TaggedHandle<TextureTag, TypeID::Texture>;


using FGBufferHandle = TaggedTHandle<BufferTag, TypeID::Buffer>;
using FGTextureHandle = TaggedTHandle<TextureTag, TypeID::Texture>;



// REQUIRES GET_DATA()
#define DEFINE_HANDLE_HASH(hashable)                    \
template<>                                              \
struct std::hash<hashable> {                          \
    size_t operator()(const hashable& h) const {      \
        return std::hash<uint64_t>{}(h.get_data());     \
    }                                                   \
};


DEFINE_HANDLE_HASH(Handle);
DEFINE_HANDLE_HASH(THandle);
