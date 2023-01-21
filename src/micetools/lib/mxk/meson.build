mxklib = static_library(
    'mxk',
    sources: [
        'mxk.c',
        'mxkCrypt.c',
        'mxkPacket.c',
        'mxkTransport.c',
    ],
    include_directories: [
        openssl_inc,
    ],
    dependencies: [openssl_lib],
)
