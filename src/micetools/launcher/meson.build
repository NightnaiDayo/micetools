rc = import('windows').compile_resources('mice.rc', depend_files: mice_ico)
executable(
    'mice',
    win_subsystem: subsystem,
    sources: [
        'locate.c',
        'spawn.c',
        'main.c',
        rc,
    ],
    link_with: [
        mice_lib,
        mxklib,

        # Madoka service emulation
        mxk,
        dummykeychip,
        dummymaster,
        dummyinstaller,
    ],
)
