dependencies = []
link_with = [libpcp, amiDebug]
sources = [
    'dummyinstaller.c',
]

dummyinstaller = static_library(
    'dummyinstaller',
    sources: sources,
    link_with: link_with,
)
executable(
    'dummyinstaller',
    win_subsystem: subsystem,
    sources: ['main.c'] + sources,
    link_with: link_with,
    dependencies: dependencies,
)
