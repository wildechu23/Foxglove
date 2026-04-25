#pragma once

#include "foxglove/memory/allocator.h"
#include "foxglove/core/handle.h"

#include <vector>

#include <iostream>

class FGBuffer;
class FGTexture;

template<typename T, typename HandleT>
class PoolHandleRegistry {
public:
    PoolHandleRegistry() = default;
    ~PoolHandleRegistry() = default; 

    template <class ...TArgs>
    HandleT create(TArgs&&... args) {
        void* allocation = m_allocator.allocate();
        T* resource_ptr = new(allocation) T(std::forward<TArgs>(args)...);
    
        HandleT handle;
        if(!m_free.empty()) {
            uint32_t index = m_free.back();
            m_free.pop_back();
            handle = HandleT(index, 0);
        } else {
            handle = HandleT(static_cast<uint32_t>(m_array.size()), 0);
        }
        m_array.push_back(resource_ptr);
        return handle;
    }

    template <class ...TArgs>
    void destroy(HandleT handle, TArgs&&... args) {
        if(!is_valid(handle)) { return; }
        uint32_t index = handle.get_index();
        T* ptr = m_array[index];
        if(ptr) {
            ptr->destroy(std::forward<TArgs>(args)...);
            m_allocator.deallocate(ptr);
            m_array[index] = nullptr;
            m_free.push_back(index);
        }
    }

    void reset() {
        m_allocator.reset();
        m_array.clear();
    }

    T* get(HandleT handle) {
        if(!is_valid(handle)) { return nullptr; }
        return m_array[handle.get_index()];
    }

    bool is_valid(HandleT handle) {
        return handle.is_valid() &&
            handle.get_index() < m_array.size();
    }
private:
    std::vector<T*> m_array;
    std::vector<uint32_t> m_free;
    PoolAllocator m_allocator;
};

// use memory allocator so pointers are valid on resize
// does not expect memory
template<typename T, typename HandleT>
class LLHandleRegistry {
public:
    LLHandleRegistry() = default;
    ~LLHandleRegistry() = default;

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

    // push to deletion queue before reset so no leak
    void reset() {
        m_allocator.reset();
        m_array.clear();
    }

    T* get(HandleT handle) {
        if(!is_valid(handle)) { return nullptr; }
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
