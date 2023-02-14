dependencies = []
link_with = [inih.get_variable('lib_inih'), libpcp]


rc = import('windows').compile_resources('micekeychip.rc', depend_files: micekeychip_ico)
sources = [
    'main.c',
    'mxk.c',
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
)
executable(
    'micekeychip',
    win_subsystem: subsystem,
    sources: sources,
    link_with: link_with,
    dependencies: dependencies,
)
