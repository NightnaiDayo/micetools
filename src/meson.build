inih = subproject('inih_dep')
detours = subproject('detours')
cimgui = subproject('cimgui_dep', default_options: [
    'win32=enabled',
    'dx9=enabled',
    'glut=enabled',
    'opengl3=enabled',
])

libs_dir = meson.current_source_dir()

incdir = [
    include_directories('micetools'),
]

subdir('micetools')
