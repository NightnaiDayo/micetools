project('detours', 'cpp',
    version: '4.0.1',
    license: 'MIT',
)

add_project_arguments(
    '/W4', '/WX', '/Zi', '/MT', '/Gy', '/Gm-', '/Zl', '/Os',
    '/DWIN32_LEAN_AND_MEAN',
    '/D_WIN32_WINNT=0x501',

    language: 'cpp',
)

include_dirs = include_directories('include')

detours = static_library(
    'detours',
    sources: [
        'src/detours.cpp',
        'src/modules.cpp',
        'src/disasm.cpp',
        'src/image.cpp',
        'src/creatwth.cpp',
        'src/disasm.cpp',
        'src/disasm.cpp',
        'src/disasm.cpp',
        'src/disasm.cpp',
        'src/disasm.cpp',
    ],
    include_directories: include_dirs
)
detours_dep = declare_dependency(include_directories: include_dirs, link_with: detours)
