#include "foxglove/vulkan/command_buffer.h"

// user must define shader
void CommandBuffer::bind_compute_shader(ComputeShader* shader) {
    // vkcmdbindpipeline
    // vkcmdbinddescriptorsets
    

}


void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
    /*
     * 1. initialize descriptor layouts
     * 2. create compute pipeline
     * 3. set current compute pipeline
     * 4. allocate descriptor sets
     * 5. swap descriptor sets
     */
}
