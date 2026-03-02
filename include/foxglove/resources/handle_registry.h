#pragma once

#include "foxglove/renderer/framegraph_pass.h"
#include "foxglove/memory/allocator.h"

// use memory allocator so pointers are valid on resize
// does not expect memory
template<typename T, typename HandleT>
class FGHandleRegistry {
public:
    FGHandleRegistry() {}
    ~FGHandleRegistry() { reset(); }

    /*
    HandleT add(T resource) {
        m_array.push_back(resource);
        return HandleT(m_array.size() - 1);
    }*/
    
    template <class ...TArgs>
    HandleT create(TArgs&&... args) {
        void* allocation = m_allocator.allocate(sizeof(T));
        T* resource_ptr = new(allocation) T(std::forward<TArgs>(args)...);
        m_array.push_back(resource_ptr);
        return HandleT(m_array.size() - 1);
    }

    void reset() {
        m_allocator.reset();
        m_array.clear();
    }

    T* get(HandleT handle) {
        if(!is_valid(handle)) return nullptr;
        return m_array[handle.get_index()];
    }

    bool is_valid(HandleT handle) {
        return handle.is_valid() &&
            handle.get_index() < m_array.size();
    }
private:
    std::vector<T*> m_array;
    LLAllocator m_allocator;
};

using FGBufferRegistry = FGHandleRegistry<FGBuffer, BufferHandle>;
using FGTextureRegistry = FGHandleRegistry<FGTexture, TextureHandle>;
