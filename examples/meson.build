testexe = executable(
  'testexe',
  'main.cpp',
  include_directories: [inc, vma_inc, spirv_reflect_inc],
  link_with: foxglove
)

test('foxglove test', testexe)
