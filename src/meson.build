subdir('core')
subdir('vulkan')
subdir('window')
subdir('renderer')
subdir('resources')
subdir('memory')
subdir('framegraph')

sources = [
  core_sources,
  vulkan_sources,
  window_sources,
  renderer_sources,
  resources_sources,
  memory_sources,
  fg_sources,
]

foxglove = library(
  'foxglove',
  sources,
  include_directories: inc,
  dependencies: [library_deps, third_party_deps],
  install: true
)

