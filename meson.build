project('foxglove', 'cpp')

add_global_arguments('-DSOME_TOKEN=value', language : 'cpp')

vulkan_dep = dependency('vulkan', required: true)
glfw_dep = dependency('glfw3', required: true)
glm_dep = dependency('glm', required: true)

inc = include_directories('include')

# third party
subdir('third_party')
subdir('include')
subdir('src')
subdir('examples')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries: foxglove,
  version: '0.1',
  name: 'libfoxglove',
  description: 'compile pass engine'
)

