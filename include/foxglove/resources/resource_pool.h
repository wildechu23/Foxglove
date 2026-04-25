#pragma once

#include "foxglove/core/handle.h"
#include "foxglove/resources/resource.h"

#include <vector>

// NOTE: DATA IS DELETED AND REMADE EVERY TIME
// CAN BE OPTIMIZED
template<typename T, typename HandleT>
class ResourcePool {
public:
    template <class ...TArgs>
    HandleT create(TArgs&&... args) {
        if(!m_free_list.empty()) {
            // reuse free slot from end
            uint32_t index = m_free_list.back();
            m_free_list.pop_back();

            Entry& entry = m_entries[index];
            entry.allocated = true;
            entry.gen++;
            entry.data = T(std::forward<TArgs>(args)...);

            return HandleT{ index, entry.gen };
        }
        
        // if no slots are free add a new entry
        uint32_t index = m_entries.size();
        m_entries.push_back({ 
            T(std::forward<TArgs>(args)...),
            1,
            true
        });

        return HandleT{ index, 1 };
    }

    template <class ...TArgs>
    void cleanup(TArgs&&... args) {
        for(Entry& e : m_entries) {
            if(e.allocated) {
                e.data.destroy(std::forward<TArgs>(args)...);
                e.allocated = false;
            }
        }
    }

    template <class ...TArgs>
    void destroy(HandleT handle, TArgs&&... args) {
        if(is_valid(handle)) {
            Entry& entry = m_entries[handle.get_index()];
            entry.data.destroy(std::forward<TArgs>(args)...);
            entry.allocated = false;
            m_free_list.push_back(handle.get_index());
        }
        // ELSE TRYING TO FREE INVALID HANDLE
    }

    // SHOULD RETURN REFERENCE?
    T* get(HandleT handle) {
        if(!is_valid(handle)) return nullptr;
        return &(m_entries[handle.get_index()].data);
    }

    bool is_valid(HandleT handle) const {
        return handle.is_valid() &&
            handle.get_index() < m_entries.size() &&
            m_entries[handle.get_index()].gen == handle.get_gen() &&
            m_entries[handle.get_index()].allocated;
    };
protected:
    struct Entry {
        T data;
        uint8_t gen;
        bool allocated;
    };

    std::vector<Entry> m_entries;
    std::vector<uint32_t> m_free_list;
    //uint32_t m_capacity;
};

using BufferPool = ResourcePool<BufferResource, BufferHandle>; 
using TexturePool = ResourcePool<TextureResource, TextureHandle>; 
