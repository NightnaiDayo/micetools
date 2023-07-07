mice_lib = static_library(
    'mice',
    sources: [
        'exe.c',
        'log.c',
        'patch.c',
        'ringbuf.c',
        'config.c',
        'kcf.c',
        'des.c',
        'blowfish.c',
        'solitaire.c',
        'spad.c',
        'dmi.c',
        'ipc.c',
        'serial.c',
    ],
    link_with: [
        inih.get_variable('lib_inih'),
        util_lib,
        amiCrc,
    ],
    include_directories: [
        openssl_inc,
    ],
)
