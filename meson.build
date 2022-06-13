project('micetools', 'c')

if (host_machine.cpu_family() == 'x86')
    add_project_arguments('-DMICE_WIN32', language: 'c')
endif

add_project_arguments(
    '-DWIN32_LEAN_AND_MEAN',  # Strip out headers we don't really need
    '-D_WIN32_WINNT=_WIN32_WINNT_WINXP',  # hahahahaha I hate it
    # '/O1',  # Optimise size
    language: 'c',
)

subdir('assets')
subdir('src')
