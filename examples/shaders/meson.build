fs = import('fs')

glslc = find_program(
    'glslc',
    required: true,
    dirs: [
        '/usr/bin',
        '/usr/local/bin',
    ]
)

shader_sources = files([
    'gradient.comp',
    'colored_triangle.vert',
    'colored_triangle.frag',
])


spirv_targets = []
foreach shader : shader_sources
    name = fs.name(shader)
    
    target = custom_target(
        input: shader,
        output: '@0@.spv'.format(name),
        command: [
            glslc, '@INPUT@',
            '-o', '@OUTPUT@'
        ],
    )
    
    spirv_targets += target
endforeach
