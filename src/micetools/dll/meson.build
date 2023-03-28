subdir('drivers')
subdir('devices')
subdir('hooks')
subdir('gui')

shared_library(
    'mice',
    name_prefix: '',
    vs_module_defs: 'mice.def',
    sources: [
        'util/misc.c',
        'util/hook.c',
        'util/path.c',

        drivers_files,
        devices_files,
        hooks_files,
        gui_files,

        'comdevice.c',
        'games.c',

        'dllmain.c',
    ],
    link_with: [
        dmi_lib,
        mice_lib,
        amiTimer,
        amiMd5,
        mxk,
        mxklib,
    ],
    include_directories: [
        openssl_inc,
    ],
    dependencies: [
        cimgui.get_variable('cimgui_dep'),
        # freeglut_lib,  # forces the use of freeglut.dll!
    ]
)
