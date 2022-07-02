project('micetools', 'c', default_options: [
    'buildtype=minsize',
    # ! Tobble /\ and \/ when building for XP (minsize=normal, static=XP)
    # 'b_vscrt=static_from_buildtype',
    'warning_level=2',
])

winxp = false
subsystem = 'console,5.01'

if (host_machine.cpu_family() == 'x86')
    add_project_arguments('-DMICE_WIN32', language: 'c')
endif

add_project_arguments(
    '/DWIN32_LEAN_AND_MEAN',  # Strip out headers we don't really need
    '/D_WIN32_WINNT=_WIN32_WINNT_WINXP',  # hahahahaha I hate it
    language: 'c',
)
if winxp
    add_project_arguments(
        '/D_USING_V140_SDK71_',
        '/DSFML_STATIC',
        '/DYNAMICBASE:NO',
        language: 'c',
    )
endif

subdir('assets')
subdir('src')
