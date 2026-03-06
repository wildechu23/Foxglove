subdir('core')
subdir('vulkan')
subdir('window')
subdir('renderer')
subdir('resources')
subdir('memory')

sources = [
  core_sources,
  vulkan_sources,
  window_sources,
  renderer_sources,
  resources_sources,
  memory_sources,
]

foxglove = shared_library(
  'foxglove',
  sources,
  include_directories: [inc, include_directories('.'), vkb_inc, vma_inc],
  link_with: vkbootstrap_lib,
  dependencies: [vulkan_dep, glfw_dep, glm_dep],
  install: true
)

