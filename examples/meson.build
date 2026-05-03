subdir('shaders')

testexe = executable('testexe',
  'main.cpp',
  include_directories: inc,
  link_with: foxglove,
  dependencies: [vulkan_dep, third_party_deps], 
  sources: spirv_targets,
)

test_env = environment()
test_env.set('TEST_SOURCE_DIR', meson.current_source_dir())

test('foxglove test',
  find_program('switcherooctl.sh'),
  args: ['launch', '-g', '1', testexe.full_path()],
  env: test_env,
  workdir: meson.current_build_dir(),
  depends: testexe
)
