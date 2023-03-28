link_with = [libpcp, amiDebug, util_lib]
sources = [
    'callbacks-stub.c',
    'callbacks-crypto.c',
    'dummykeychip.c',
]

dummykeychip = static_library(
    'dummykeychip',
    sources: sources,
    link_with: link_with,
    include_directories: [openssl_inc],
    dependencies: [openssl_lib],
)
executable(
    'dummykeychip',
    win_subsystem: subsystem,
    sources: ['main.c'] + sources,
    link_with: link_with,
    include_directories: [openssl_inc],
    dependencies: [openssl_lib],
)
