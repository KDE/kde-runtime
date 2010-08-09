project(kdbgwin)

include(KDE4Defaults)

add_definitions (${KDE4_DEFINITIONS})

set(kdbgwin_SRCS
    common.h
    main.cpp
    process.h
    process.cpp
    abstract_generator.h
    abstract_generator.cpp
    callbacks.h
    callbacks.cpp
    outputters.h
    outputters.cpp
)

if ( MINGW )
    kde4_add_executable(kdbgwin ${kdbgwin_SRCS} mingw_generator.h mingw_generator.cpp)
    target_link_libraries(kdbgwin ${KDE4_KDECORE_LIBS} dbghelp psapi bfd iberty intl)
endif ( MINGW )
if ( MSVC )
    kde4_add_executable(kdbgwin ${kdbgwin_SRCS} msvc_generator.h msvc_generator.cpp)
    target_link_libraries(kdbgwin ${KDE4_KDECORE_LIBS} dbghelp psapi)
endif ( MSVC )

install(TARGETS kdbgwin DESTINATION ${LIBEXEC_INSTALL_DIR})