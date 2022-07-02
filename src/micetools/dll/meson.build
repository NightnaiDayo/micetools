subdir('drivers')
subdir('devices')
subdir('hooks')

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

        'comdevice.c',

        'dllmain.c',
    ],
    link_with: [
        dmi_lib,
        mice_lib,
    ]
)
