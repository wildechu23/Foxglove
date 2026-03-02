#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <new>

struct Chunk {
    Chunk* next;
};

struct BlockStart {
    Chunk* chunks;
    BlockStart* next;
};

class PoolAllocator {
public:
    PoolAllocator(size_t chunk_size, size_t initial_blocks = 1);
    ~PoolAllocator();

    void* allocate();
    void deallocate(void* ptr);
    void reset();
private:
    size_t m_chunk_size;
    size_t m_chunks_per_block = 64;
    

    Chunk* allocate_block();

    // alloc ptr
    Chunk* m_alloc;
    BlockStart* m_block_starts;
};

// does not handle alloc_size > page_size
class LLAllocator {
public:
    // page size from unreal
    LLAllocator(size_t page_size = 64 * 1024);
    ~LLAllocator();
    
    // Page of memory is tagged with the next page and its size
    struct Page {
        Page* next;
        size_t data_size;

        uint8_t* data() {
            return reinterpret_cast<uint8_t*>(this) + sizeof(Page);
        }
    };

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void reset();
    
    uint8_t* align(uint8_t* ptr, size_t alignment); 
private:
    void allocate_page();
    
    Page* m_top_page;
    uint8_t* m_top;
    uint8_t* m_end;

    // remember to add header size when allocating
    size_t m_page_size;
};
