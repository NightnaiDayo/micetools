mice_lib = static_library(
    'mice',
    sources: [
        'exe.c',
        'patch.c',
        'crc.c',
        'ringbuf.c',
        'config.c',
    ],
    link_with: [
        inih.get_variable('lib_inih'),
        json_lib,
    ]
)
