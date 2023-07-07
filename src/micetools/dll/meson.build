subdir('drivers')
subdir('devices')
subdir('hooks')
subdir('gui')

shared_library(
    get_option('win64') ? 'mice64' : 'mice',
    name_prefix: '',
    vs_module_defs: get_option('win64') ? 'mice64.def' : 'mice.def',
    sources: [
        'dllStub/amv.c',
        'dllStub/ati.c',

        'util/misc.c',
        'util/hook.c',
        'util/path.c',

        drivers_files,
        devices_files,
        hooks_files,
        gui_files,

        'comdevice.c',
        'games.c',
        'input.c',
        'micefs.c',

        'dllmain.c',
    ],
    link_with: [
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
        detours.get_variable('detours_dep'),
        # freeglut_lib,  # forces the use of freeglut.dll!
    ]
)
