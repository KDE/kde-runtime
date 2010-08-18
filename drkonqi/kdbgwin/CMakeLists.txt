project(kdbgwin)

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

set(COMMON_LIBS ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} dbghelp psapi shlwapi)

if ( MINGW )
    kde4_add_executable(kdbgwin ${kdbgwin_SRCS} mingw_generator.h mingw_generator.cpp)
    target_link_libraries(kdbgwin ${COMMON_LIBS} bfd iberty intl)
endif ( MINGW )
if ( MSVC )
    kde4_add_executable(kdbgwin ${kdbgwin_SRCS} msvc_generator.h msvc_generator.cpp)
    target_link_libraries(kdbgwin ${COMMON_LIBS})
endif ( MSVC )

install(TARGETS kdbgwin DESTINATION ${LIBEXEC_INSTALL_DIR})
