project('foxglove', 'cpp',
  default_options : [
    'cpp_std=c++20',
    'default_library=static'
  ])

cmake = import('cmake')
fastgltf_sub = cmake.subproject('fastgltf')
fastgltf_dep = fastgltf_sub.dependency('fastgltf')

vulkan_dep = dependency('vulkan', required: true)
glfw_dep = dependency('glfw3', required: true)
glm_dep = dependency('glm', required: true)

library_deps = [
  vulkan_dep,
  glfw_dep,
  glm_dep,
  fastgltf_dep
]

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
  description: 'compile pass engine',
)

