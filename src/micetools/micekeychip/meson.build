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

executable(
    'micekeychip',
    win_subsystem: 'console',
    sources: [
        'main.c',
        'mxk.c',
        'callbacks/appboot.c',
        'callbacks/billing.c',
        'callbacks/crypto.c',
        'callbacks/misc.c',
        'callbacks/tracedata.c',
        'callbacks/storage.c',
    ],
    link_with: link_with,
    dependencies: dependencies,
)
