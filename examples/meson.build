subdir('shaders')

testexe = executable('testexe',
  'main.cpp',
  include_directories: inc,
  link_with: foxglove,
  dependencies: [vulkan_dep, third_party_deps], 
  sources: spirv_targets,
)


test('foxglove test', testexe,
  workdir: meson.current_build_dir())
