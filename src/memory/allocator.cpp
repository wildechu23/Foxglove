#include "foxglove/memory/allocator.h"

PoolAllocator::PoolAllocator(size_t chunk_size, size_t initial_blocks)
    : m_chunk_size(chunk_size) {
    for(size_t i = 0; i < initial_blocks; i++) {
        allocate_block();
    }
    
    if(m_block_starts != nullptr) {
        m_alloc = m_block_starts->chunks;
    }
}

PoolAllocator::~PoolAllocator() {
    reset();
}

Chunk* PoolAllocator::allocate_block() {
    size_t block_size = m_chunk_size * m_chunks_per_block;
    Chunk* block_begin = static_cast<Chunk*>(::operator new(block_size));
   
    // if you want to avoid casts, you can define Chunk as union of
    // fixed array type and pointer type
    Chunk* chunk = block_begin;
    for(size_t i = 0; i < m_chunks_per_block - 1; i++) {
        chunk->next = reinterpret_cast<Chunk*>(
                reinterpret_cast<char*>(chunk) + m_chunk_size);
        chunk = chunk->next;
    }
    chunk->next = m_alloc; 

    // add blockstart to front
    BlockStart* bs = new BlockStart;
    bs->chunks = block_begin;
    bs->next = m_block_starts;
    m_block_starts = bs;

    m_alloc = block_begin;
    return block_begin;
}

void* PoolAllocator::allocate() {
    if(m_alloc == nullptr) {
        allocate_block();
    }
    Chunk* free = m_alloc;
    m_alloc = m_alloc->next;
    return free;
}

// add chunk to beginning of m_alloc free list
void PoolAllocator::deallocate(void* chunk) {
    reinterpret_cast<Chunk*>(chunk)->next = m_alloc;
    m_alloc = reinterpret_cast<Chunk*>(chunk);
}

void PoolAllocator::reset() {
    BlockStart* bs = m_block_starts;
    while(bs != nullptr) {
        BlockStart* next = bs->next;
        ::operator delete(bs->chunks);
        delete bs;
        bs = next;
    }
    m_block_starts = nullptr;
    m_alloc = nullptr;
}

#include <iostream>

LLAllocator::LLAllocator(size_t page_size) : m_page_size(page_size) {
    m_top_page = nullptr;
    m_top = nullptr;
    m_end = nullptr;
}

LLAllocator::~LLAllocator() {
    reset();
}

// aligment must be power of 2
uint8_t* LLAllocator::align(uint8_t* ptr, size_t alignment) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<uint8_t*>(aligned);
}

void LLAllocator::allocate_page() {
    size_t total = sizeof(Page) + m_page_size;
    Page* new_page = static_cast<Page*>(::operator new(total));
    
    new_page->data_size = m_page_size;
    new_page->next = m_top_page; 
    m_top_page = new_page;

    m_top = new_page->data();
    m_end = m_top + new_page->data_size;
}

// TODO: catch allocate size > page_size error
void* LLAllocator::allocate(size_t size, size_t alignment) {
    assert(size <= m_page_size);

    if(m_top_page == nullptr) {
        allocate_page();
    }

    uint8_t* aligned = align(m_top, alignment);
    uint8_t* new_top = aligned + size;

    // if out of capacity
    if(new_top > m_end) {
        allocate_page();
        aligned = align(m_top, alignment); // necessary?
        new_top = aligned + size;
    }
    
    m_top = new_top;
    return aligned;
}


void LLAllocator::reset() {
    while(m_top_page != nullptr) {
        Page* out = m_top_page;
        m_top_page = m_top_page->next;
        ::operator delete(out);
    }

    m_top = nullptr;
    m_end = nullptr;
}
