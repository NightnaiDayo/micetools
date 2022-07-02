rc = import('windows').compile_resources('micepatch.rc', depend_files: micepatch_ico)
micepatch = executable(
    'micepatch',
    win_subsystem: subsystem,
    sources: [
        'main.c',
        rc,
    ],
    link_with: [
        mice_lib,
    ],
    link_args: [
        '/MANIFEST:EMBED',
        '/MANIFESTUAC:level="asInvoker" uiAccess="false"',
    ]
)
