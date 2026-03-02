Foxglove

Foxglove is a GPU-based rendergraph 3D renderer.


core/

renderer/
- renderer

vulkan/
- swapchain
- vulkan context

window/
- window


Format Rules:
- Always use "init" and "cleanup"
- Destructor call cleanup
- void* fun(void* a, int& b) {}
- m_ for private variables


