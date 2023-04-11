dummykeychip = static_library(
    'dummykeychip',
    sources: [
        'callbacks-stub.c',
        'callbacks-crypto.c',
        'dummykeychip.c',
    ],
    link_with: [libpcp, amiDebug, util_lib],
    include_directories: [openssl_inc],
    dependencies: [openssl_lib],
)
