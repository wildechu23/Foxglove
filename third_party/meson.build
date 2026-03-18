vkb_inc = include_directories('vkbootstrap')

vkbootstrap_lib = static_library(
  'vkbootstrap',
  files('vkbootstrap/VkBootstrap.cpp'),
  include_directories: vkb_inc
)

vma_inc = include_directories('vma')

spirv_reflect_inc = include_directories('spirv-reflect')

third_party_inc = [vkb_inc, vma_inc, spirv_reflect_inc]
