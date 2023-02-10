project('cimgui', 'cpp',
    version: '1.89',
    license: 'MIT',
)

subdir('imgui')

cimgui = static_library('cimgui',
    ['cimgui.cpp'],
    dependencies: [imgui_dep]
)

include_dirs = include_directories('.')

cimgui_dep = declare_dependency(include_directories: include_dirs, link_with: cimgui)
