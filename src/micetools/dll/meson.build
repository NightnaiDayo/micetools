shared_library(
    'mice',
    name_prefix: '',
    vs_module_defs: 'mice.def',
    sources: [
        'dllmain.c',
        'files.c',
        'drivers/columba.c',
        'drivers/mxsram.c',
        'drivers/mxsuperio.c',
        'drivers/mxjvs.c',
        'drivers/mxsmbus.c',
        'drivers/mxhwreset.c',
        'setupapi.c',
        'com.c',
        'comdevice.c',
        'processes.c',
        'network.c',
    ],
    link_with: [
        dmi_lib,
        mice_lib,
    ],
)
