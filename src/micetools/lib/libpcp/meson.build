compiler = meson.get_compiler('c')
ws2 = compiler.find_library('ws2_32', required: true)

sources = [
    'libpcp.c',
    'pcpt.c',
    'pcpp.c',
    'pcpa.c',
    'pcp.c',
    'util.c',
]

libpcp = static_library(
    'pcp',
    name_suffix: 'lib',
    sources: sources,
    dependencies: [
        ws2,
    ],
    link_with: [
        amlib,
    ],
)
