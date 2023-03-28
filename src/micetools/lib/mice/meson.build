mice_lib = static_library(
    'mice',
    sources: [
        'exe.c',
        'log.c',
        'patch.c',
        'ringbuf.c',
        'config.c',
    ],
    link_with: [
        inih.get_variable('lib_inih'),
        util_lib,
        amiCrc,
    ],
)
