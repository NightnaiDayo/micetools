dependencies = []
link_with = [libpcp, amiDebug]
sources = [
    'dummymaster.c',
]

dummymaster = static_library(
    'dummymaster',
    sources: sources,
    link_with: link_with,
)
executable(
    'dummymaster',
    win_subsystem: subsystem,
    sources: ['main.c'] + sources,
    link_with: link_with,
    dependencies: dependencies,
)
