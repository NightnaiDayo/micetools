rc = import('windows').compile_resources('micemaster.rc', depend_files: micekeychip_ico)
executable(
    'micemaster',
    win_subsystem: subsystem,
    sources: [
        'config.c',
        'main.c',
        'mxm.c',
        'mxmEventLog.c',
        'appLauncher.c',
        'callbacks/foreground.c',
        'callbacks/logs.c',
        'callbacks/misc.c',
        rc,
    ],
    link_with: [
        inih.get_variable('lib_inih'),
        libpcp,
        amiDebug,
        amDongle,
        amInstall,
    ],
    include_directories: incdir,
    dependencies: [],
)
