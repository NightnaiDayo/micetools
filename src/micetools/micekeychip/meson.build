dependencies = []
link_with = [inih.get_variable('lib_inih')]

# Depending on how we're getting access to libpcp, we need some extra work here
if libpcp_is_static
    link_with += libpcp
else
    link_with += amlib
    dependencies += libpcp
    dependencies += meson.get_compiler('c').find_library('ws2_32')
endif

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
