project( DRKONQUING )

find_package(KDE4 REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

add_subdirectory( presets ) 
add_subdirectory( pics ) 
add_subdirectory( debuggers ) 
add_subdirectory( tests ) 

set(drkonqi_SRCS 
   main.cpp 
   krashconf.cpp 
   krashadaptor.cpp
   bugzillalib.cpp
   drkonqidialog.cpp
   crashinfo.cpp
   aboutbugreportingdialog.cpp
   drkonqiassistantpages_base.cpp
   drkonqiassistantpages_bugzilla.cpp
   drkonqibugreport.cpp
   usefulnessmeter.cpp
   getbacktracewidget.cpp
   backtracegenerator.cpp 
   backtraceparser.cpp
)

kde4_add_ui_files(drkonqi_SRCS getbacktracewidget.ui)

# if BACKTRACE_PARSER_DEBUG is enabled, it will show both the
# parsed and the unparsed backtrace in the backtrace widget.
# Comment this out for release.
add_definitions(-DBACKTRACE_PARSER_DEBUG)

kde4_add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi ${KDE4_KIO_LIBS})

install(TARGETS drkonqi DESTINATION ${LIBEXEC_INSTALL_DIR})
