project('micetools', 'c', default_options: [
    'buildtype=minsize',
    # ! Toggle /\ and \/ when building for XP (minsize=normal, static=XP)
    # 'b_vscrt=static_from_buildtype',
    'warning_level=3',
])

winxp = false
subsystem = 'console,5.01'

if (host_machine.cpu_family() == 'x86')
    add_project_arguments('-DMICE_WIN32', language: 'c')
endif

add_project_arguments(
    '/DWIN32_LEAN_AND_MEAN',  # Strip out headers we don't really need
    '/D_WIN32_WINNT=_WIN32_WINNT_WINXP',  # hahahahaha I hate it

    '/wd4706',  # assignment within conditional expression
    '/wd4214',  # windns.h: nonstandard extension used: bit field types other than int
    '/wd4201',  # ewfapi.h: nameless struct/union
    '/wd4200',  # zero-sized arrays
    '/wd4152',  # hooks: function/data pointer conversion in expression
    '/wd4100',  # basically every hook causes "unreferenced formal parameter"
    '/wd4206',  # empty C files

    '/wd4189',  # lots of keychip functions aren't 100% implemented yet

    '/we4047',  # ... differs in levels of indirection from ...
    '/we4057',  # ... differs in levels of indirection (slightly) from ...
    '/we4024',  # ... different types for formal and actual paramter ...
    '/we4013',  # ... undefined; assuming extern returning int

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
