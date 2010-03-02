project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#search packages used by KDE
set(KDE_MIN_VERSION "4.4.59")  # for the < 4.2 macro
find_package(KDE4 4.4.59 REQUIRED)
find_package(Strigi REQUIRED)
# I don't see any package requiring it. please explain why this dependency
# is necessary (dirk)
#find_package(KdepimLibs REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

set(SOPRANO_MIN_VERSION "2.3.70")
macro_optional_find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk Libraries" "kdelibs" FALSE "" "Required to build Nepomuk.")
macro_log_feature(SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND "Soprano Virtuoso Backend" "Virtuoso storage backend for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano Virtuoso backend is required to run the Nepomuk semantic desktop system.")
macro_log_feature(SOPRANO_PLUGIN_RAPTORPARSER_FOUND "Soprano Raptor Parser" "RDF parser plugin for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano raptor parser plugin is required to build the Nepomuk semantic desktop system.")
macro_log_feature(SOPRANO_PLUGIN_REDLANDBACKEND_FOUND "Soprano Redland Backend" "Redland storage backend for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano redland backend is required to build the Nepomuk semantic desktop system.")

macro_optional_find_package(SLP)
macro_log_feature( SLP_FOUND
  "OpenSLP" "SLP (Service Location Protocol) implementation" "http://www.openslp.org/" FALSE
  "" "Provides SLP support in the network:/ kioslave."
)

set(LIBATTICA_MIN_VERSION "0.1.1")
find_package(LibAttica)
macro_log_feature(LIBATTICA_FOUND "libattica" "A library to access Open Collaboration Service providers" "http://websvn.kde.org/trunk/kdesupport/attica/" TRUE "" "Required for Get Hot New Stuff."
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
   add_subdirectory(solid-device-automounter)
endif ( UNIX )
if(Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_PLUGIN_REDLANDBACKEND_FOUND AND SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)
  macro_optional_add_subdirectory(nepomuk)
endif(Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_PLUGIN_REDLANDBACKEND_FOUND AND SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)

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
add_subdirectory(localization)
add_subdirectory(kde-menu)
macro_optional_add_subdirectory(doc)

# Artwork
add_subdirectory(desktoptheme)
add_subdirectory(pics)

# KDE integration for attica
if(LIBATTICA_FOUND)
  macro_optional_add_subdirectory(attica)
endif(LIBATTICA_FOUND)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

########### install files ###############
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/kde4 DESTINATION ${BIN_INSTALL_DIR})
