executable(
    'micepatch',
    win_subsystem: 'console',
    sources: [
        'main.c',
    ],
    link_with: [
        mice_lib,
        json_lib,
    ],
    link_args: [
        '/MANIFEST:EMBED',
        '/MANIFESTUAC:level="asInvoker" uiAccess="false"',
    ]
)
