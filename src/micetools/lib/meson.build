subdir('util')  # This is the only lib that should ever be link_with'd by another lib
subdir('am')
subdir('mice')
subdir('dmi')
subdir('json')

fs = import('fs')
# Handle the fact we aren't distributing the libpcp source
if fs.is_dir('libpcp')
    subdir('libpcp')
    libpcp_is_static = true
else
    libpcp = meson.get_compiler('c').find_library('libpcp', dirs: libs_dir, required: false)
    assert(libpcp.found(), 'Unable to locate libpcp.lib. Make sure to place it in the src/ folder prior to building.')
    libpcp_is_static = false
endif
