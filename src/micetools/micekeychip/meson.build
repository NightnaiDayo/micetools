dependencies = [openssl_lib]
link_with = [inih.get_variable('lib_inih'), libpcp, mice_lib]


rc = import('windows').compile_resources('micekeychip.rc', depend_files: micekeychip_ico)
sources = [
    'mxk.c',
    'config.c',
    'callbacks/appboot.c',
    'callbacks/billing.c',
    'callbacks/crypto.c',
    'callbacks/misc.c',
    'callbacks/tracedata.c',
    'callbacks/storage.c',
    rc,
]
mxk = static_library(
    'mxk',
    sources: sources,
    link_with: link_with,
    dependencies: dependencies,
    include_directories: [
        openssl_inc,
    ],
)
executable(
    'micekeychip',
    win_subsystem: subsystem,
    sources: sources + ['main.c'],
    link_with: link_with,
    dependencies: dependencies,
    include_directories: [
        openssl_inc,
    ],
)
