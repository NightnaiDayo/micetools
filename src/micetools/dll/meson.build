subdir('drivers')
subdir('devices')
subdir('hooks')
subdir('gui')

shared_library(
    'mice',
    name_prefix: '',
    vs_module_defs: 'mice.def',
    sources: [
        'util/log.c',
        'util/hook.c',

        drivers_files,
        devices_files,
        hooks_files,
        gui_files,

        'comdevice.c',

        'dllmain.c',
    ],
    link_with: [
        dmi_lib,
        mice_lib,
        amlib,
        mxklib,
    ],
    include_directories: [
        openssl_inc,
    ],
    dependencies: [cimgui.get_variable('cimgui_dep')]
)
