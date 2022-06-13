rc = import('windows').compile_resources('mice.rc', depend_files: mice_ico)
executable(
    'mice',
    win_subsystem: 'console',
    sources: [
        'locate.c',
        'exe.c',
        'main.c',
        rc,
    ],
    link_with: [
        mice_lib,
    ],
)
