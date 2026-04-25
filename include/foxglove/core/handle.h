#pragma once

#include "foxglove/core/types.h"

#include <functional>
#include <cstdint>

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

// TRANSIENT HANDLE
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

// TypeValue MUST static_cast to uint8_t
template<auto TypeValue>
class TaggedHandle : public Handle {
public:
    using EnumType = decltype(TypeValue);
    static constexpr EnumType type_id() { 
        return TypeValue;
    }
    
    TaggedHandle() : Handle(INVALID) {}

    static TaggedHandle invalid() { 
        return TaggedHandle(INVALID); 
    }

    constexpr TaggedHandle(uint32_t index, uint32_t gen) :
        Handle(static_cast<uint8_t>(TypeValue), index, gen) {}

    explicit TaggedHandle(const Handle& h) : Handle(h.m_data) {}
    
    TaggedHandle(const TaggedHandle&) = default;
    TaggedHandle& operator=(const TaggedHandle&) = default;
};

template<auto TypeValue>
class TaggedTHandle : public THandle {
public:
    using EnumType = decltype(TypeValue);
    static constexpr EnumType type_id() { 
        return TypeValue;
    }
    
    TaggedTHandle() : THandle(INVALID) {}

    static TaggedTHandle invalid() { 
        return TaggedTHandle(INVALID); 
    }

    constexpr TaggedTHandle(uint32_t index) :
        THandle(static_cast<uint8_t>(TypeValue), index) {}

    explicit TaggedTHandle(const THandle& h) : 
        THandle(h.m_data) {}
    
    TaggedTHandle(const TaggedTHandle&) = default;
    TaggedTHandle& operator=(const TaggedTHandle&) = default;
};

template<>
struct std::hash<Handle> {
    size_t operator()(const Handle& h) const {
        return std::hash<uint64_t>{}(h.get_data());
    }
};

template<>
struct std::hash<THandle> {
    size_t operator()(const THandle& h) const {
        return std::hash<uint64_t>{}(h.get_data());
    }
};

template<auto TypeValue>
struct std::hash<TaggedHandle<TypeValue>> {
    size_t operator()(const TaggedHandle<TypeValue>& h) const {
        return std::hash<uint64_t>{}(h.get_data());
    }
};

template<auto TypeValue>
struct std::hash<TaggedTHandle<TypeValue>> {
    size_t operator()(const TaggedTHandle<TypeValue>& h) const {
        return std::hash<uint64_t>{}(h.get_data());
    }
};
