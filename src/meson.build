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

foxglove = library(
  'foxglove',
  sources,
  include_directories: inc,
  dependencies: [vulkan_dep, glfw_dep, glm_dep, third_party_deps],
  install: true
)

