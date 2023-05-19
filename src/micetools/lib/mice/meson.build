mice_lib = static_library(
    'mice',
    sources: [
        'exe.c',
        'log.c',
        'patch.c',
        'ringbuf.c',
        'config.c',
        'kcf.c',
        'micefs.c',
        'des.c',
        'blowfish.c',
        'solitaire.c',
        'spad.c',
    ],
    link_with: [
        inih.get_variable('lib_inih'),
        util_lib,
        amiCrc,
    ],
)
