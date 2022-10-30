dependencies = []
link_with = [inih.get_variable('lib_inih'), libpcp]


rc = import('windows').compile_resources('micekeychip.rc', depend_files: micekeychip_ico)
executable(
    'micekeychip',
    win_subsystem: subsystem,
    sources: [
        'main.c',
        'mxk.c',
        'callbacks/appboot.c',
        'callbacks/billing.c',
        'callbacks/crypto.c',
        'callbacks/misc.c',
        'callbacks/tracedata.c',
        'callbacks/storage.c',
        rc,
    ],
    link_with: link_with,
    dependencies: dependencies,
)
