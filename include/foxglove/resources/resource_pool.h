#pragma once

#include "foxglove/resources/resource_types.h"

#include <vector>

template<typename Resource, typename ResourceHandle>
class ResourcePool {
public:
    ResourceHandle create() {
        if(!m_free_list.empty()) {
            // reuse free slot from end
            uint32_t index = m_free_list.back();
            m_free_list.pop_back();

            Entry& entry = m_entries[index];
            entry.allocated = true;
            entry.gen++;

            return { index, entry.gen };
        }
        
        // if no slots are free add a new entry
        uint32_t index = m_entries.size();
        m_entries.push_back({ Resource{}, 1, true });

        return { index, 1 };
    }

    void destroy(ResourceHandle handle) {
        if(is_valid(handle)) {
            Entry& entry = m_entries[handle.get_index()];
            entry.allocated = false;
            m_free_list.push_back(handle.get_index());
        }
        // ELSE TRYING TO FREE INVALID HANDLE
    }

    // SHOULD RETURN REFERENCE?
    Resource* get(ResourceHandle handle) {
        if(!is_valid(handle)) return nullptr;

        return &(m_entries[handle.get_index()].data);

    }

    bool is_valid(ResourceHandle handle) const {
        return handle.is_valid() &&
            handle.get_index() < m_entries.size() &&
            m_entries[handle.get_index()].gen == handle.get_gen() &&
            m_entries[handle.get_index()].allocated;
    };
protected:
    struct Entry {
        Resource data;
        uint8_t gen;
        bool allocated;
    };

    std::vector<Entry> m_entries;
    std::vector<uint32_t> m_free_list;
    //uint32_t m_capacity;
};
