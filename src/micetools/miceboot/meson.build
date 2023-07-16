executable(
    'miceprestartup',
    win_subsystem: subsystem,
    sources: [
        'mxprestartup.c',
    ],
)

executable(
    'micestartup',
    win_subsystem: subsystem,
    sources: [
        'mxstartup.c',
        'osu.c',
        'osg.c',
    ],
    link_with: [
        amLog,
        amiDebug,
        amiCrc,
        amBackup,
        amEeprom,
        amBackup,
        amCmos,
        amPlatform,
    ],
    dependencies: [
        ewfapi_lib,
    ],
)

executable(
    'mxmaster',
    win_subsystem: subsystem,
    sources: [
        'mxmaster.c',
    ],
)
