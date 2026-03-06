#pragma once

#include "foxglove/memory/allocator.h"
#include "foxglove/resources/handle.h"

#include <vector>

#include <iostream>

class FGBuffer;
class FGTexture;


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
        
        // set handle of new object via friendship
        T* resource_ptr = new(allocation) T(std::forward<TArgs>(args)...);
        HandleT handle = HandleT(static_cast<uint32_t>(
                    m_array.size()));
        resource_ptr->m_handle = handle; 
        m_array.push_back(resource_ptr);
        return handle;
    }

    void reset() {
        m_allocator.reset();
        m_array.clear();
    }

    T* get(HandleT handle) {
        if(!is_valid(handle)) {
            return nullptr;
        }
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

using FGBufferRegistry = FGHandleRegistry<FGBuffer, FGBufferHandle>;
using FGTextureRegistry = FGHandleRegistry<FGTexture, FGTextureHandle>;
