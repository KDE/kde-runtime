project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#search packages used by KDE
set(KDE_MIN_VERSION "4.3.5")  # for the < 4.2 macro
find_package(KDE4 4.3.5 REQUIRED)
find_package(Strigi REQUIRED)
# I don't see any package requiring it. please explain why this dependency
# is necessary (dirk)
#find_package(KdepimLibs REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

set(SOPRANO_MIN_VERSION "2.3.0")
macro_optional_find_package(Soprano)
macro_optional_find_package(Nepomuk)
macro_log_feature(Soprano_FOUND "Soprano" "Soprano Libraries" "kdesupport" FALSE "" "Required to build Nepomuk.")
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk Libraries" "kdelibs" FALSE "" "Required to build Nepomuk.")
if(SOPRANO_PLUGIN_SESAME2BACKEND_FOUND OR SOPRANO_PLUGIN_REDLANDBACKEND_FOUND OR SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND)
  set(SOPRANO_BACKEND_FOUND true)
endif(SOPRANO_PLUGIN_SESAME2BACKEND_FOUND OR SOPRANO_PLUGIN_REDLANDBACKEND_FOUND OR SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND)
macro_log_feature(SOPRANO_BACKEND_FOUND "Soprano Storage Backend" "Soprano Libraries" "kdesupport" FALSE "" "No Soprano storage backend found. Nepomuk will not be compiled without it. (It is highly recommended to use the Sesame2 backend for performance.)")
macro_log_feature(SOPRANO_PLUGIN_RAPTORPARSER_FOUND "Soprano raptor parser plugin" "Soprano Libraries" "kdesupport" FALSE "" "Required to run Nepomuk.")

macro_optional_find_package(SLP)
macro_log_feature( SLP_FOUND
  "OpenSLP" "SLP (Service Location Protocol) implementation" "http://www.openslp.org/" FALSE
  "" "Provides SLP support in the network:/ kioslave."
)


check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

# Build infrastructure
add_subdirectory(cmake)

# Background processes
add_subdirectory(kpasswdserver)
add_subdirectory(kglobalaccel)
add_subdirectory(knotify)
add_subdirectory(ktimezoned)
add_subdirectory(kuiserver)
add_subdirectory(kwalletd)
if ( UNIX )
   add_subdirectory(soliduiserver)
   add_subdirectory(solidautoeject)
endif ( UNIX )
if(Soprano_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_BACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)
  macro_optional_add_subdirectory(nepomuk)
endif(Soprano_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_BACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)

# Plugins/components required for basic utility
add_subdirectory(kcontrol)
add_subdirectory(kioslave)
add_subdirectory(kurifilter-plugins)
add_subdirectory(phonon)
macro_optional_add_subdirectory(renamedlgplugins)

# UI Helper applications
if ( NOT WIN32 )
  add_subdirectory(drkonqi)
endif ( NOT WIN32 )
add_subdirectory(kcmshell)
add_subdirectory(kdebugdialog)
add_subdirectory(khelpcenter)
add_subdirectory(knetattach)
add_subdirectory(knewstuff)
macro_optional_add_subdirectory(keditfiletype)
if ( UNIX )
   add_subdirectory(kdesu)
endif ( UNIX )

# Command-line tools (e.g. for shell scripts)
if ( UNIX )
   add_subdirectory(kdeeject)
endif ( UNIX )
add_subdirectory(kfile)
add_subdirectory(kiconfinder)
add_subdirectory(kioclient)
add_subdirectory(kioexec)
add_subdirectory(ktraderclient)
add_subdirectory(kreadconfig)
add_subdirectory(kmimetypefinder)
if (Q_WS_X11)
  add_subdirectory(kstart)
endif (Q_WS_X11)
add_subdirectory(kquitapp)
add_subdirectory(menu)
add_subdirectory(plasma)
add_subdirectory(solid-hardware)

# Platform-specific integration
add_subdirectory(platforms)

# Default settings, content and config
add_subdirectory(l10n)
add_subdirectory(kde-menu)
macro_optional_add_subdirectory(doc)

# Artwork
add_subdirectory(desktoptheme)
add_subdirectory(pics)
add_subdirectory(kstyles)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

########### install files ###############
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/kde4 DESTINATION ${BIN_INSTALL_DIR})
