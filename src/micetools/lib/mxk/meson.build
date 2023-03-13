mxklib = static_library(
    'mxk',
    sources: [
        'mxk.c',
        'mxkAb.c',
        'mxkDs.c',
        'mxkN2.c',
        'mxkCrypt.c',
        'mxkSmbus.c',
        'mxkPacket.c',
        'mxkTransport.c',
    ],
    include_directories: [
        openssl_inc,
    ],
    link_with: [
        amiCrc,
        amiDebug,
    ],
    dependencies: [openssl_lib],
)
