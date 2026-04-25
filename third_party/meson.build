cmake = import('cmake')

vkb_inc = include_directories('vkbootstrap')
vma_inc = include_directories('vma')
spirv_reflect_inc = include_directories('spirv-reflect')
xxhash_inc = include_directories('xxhash')

vkb_dep = declare_dependency(
  sources: files('vkbootstrap/VkBootstrap.cpp'),
  include_directories: vkb_inc,
  dependencies : [vulkan_dep]
)

vma_dep = declare_dependency(
  include_directories : vma_inc,
  dependencies : [vulkan_dep]
)

spirv_reflect_dep = declare_dependency(
  sources: files('spirv-reflect/spirv_reflect.cpp'),
  include_directories: spirv_reflect_inc,
  dependencies : [vulkan_dep]
)

xxhash_dep = declare_dependency(
  include_directories : xxhash_inc,
  compile_args: ['-DXXH_INLINE_ALL']
)

third_party_deps = [
  vkb_dep, 
  vma_dep, 
  spirv_reflect_dep,
  xxhash_dep,
]
