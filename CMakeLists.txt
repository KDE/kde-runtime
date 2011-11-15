project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

if(WINCE)
    #Needs to be set because otherwise he wouldn't find kde parts
    set(KDE4_DATA_DIR "${CMAKE_PREFIX_PATH}/share/apps")
    add_definitions(-DKDELIBS_STATIC_LIBS)
endif(WINCE)

#search packages used by KDE
set(KDE_MIN_VERSION "4.6.80")  # for the < 4.2 macro
find_package(KDE4 4.6.80 REQUIRED)
if (WIN32)
    find_package(Strigi)
else ()
    find_package(Strigi REQUIRED)
endif ()

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

set(SOPRANO_MIN_VERSION "2.6.50")
set(SDO_MIN_VERSION "0.6.50")
macro_optional_find_package(SharedDesktopOntologies ${SDO_MIN_VERSION})
macro_optional_find_package(Nepomuk)
macro_log_feature(SHAREDDESKTOPONTOLOGIES_FOUND "Shared desktop ontologies" "Desktop ontologies" "http://oscaf.sourceforge.net" FALSE ${SDO_MIN_VERSION} "Ontologies necessary for the Nepomuk semantic desktop.")
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk Libraries" "kdelibs" FALSE "" "Required to build Nepomuk.")
macro_log_feature(SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND "Soprano Virtuoso Backend" "Virtuoso storage backend for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano Virtuoso backend is required to run the Nepomuk semantic desktop system.")
macro_log_feature(SOPRANO_PLUGIN_RAPTORPARSER_FOUND "Soprano Raptor Parser" "RDF parser plugin for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano raptor parser plugin is required to build the Nepomuk semantic desktop system.")
macro_log_feature(SOPRANO_PLUGIN_REDLANDBACKEND_FOUND "Soprano Redland Backend" "Redland storage backend for Soprano" "http://soprano.sourceforge.net" FALSE "" "The Soprano redland backend is required to build the Nepomuk semantic desktop system.")

macro_optional_find_package(SLP)
macro_log_feature( SLP_FOUND
  "OpenSLP" "SLP (Service Location Protocol) implementation" "http://www.openslp.org/" FALSE
  "" "Provides SLP support in the network:/ kioslave."
)

set(LIBATTICA_MIN_VERSION "0.1.4")
find_package(LibAttica)
macro_log_feature(LIBATTICA_FOUND "libattica" "A library to access Open Collaboration Service providers" "http://websvn.kde.org/trunk/kdesupport/attica/" TRUE "" "Required for Get Hot New Stuff."
)

macro_optional_find_package(QCA2)
macro_log_feature(QCA2_FOUND "QCA2" "Support for remote plasma widgets" "http://delta.affinix.com/qca" FALSE "2.0.0" "")

check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

# Build infrastructure
add_subdirectory(cmake)

add_definitions(-DDISABLE_NEPOMUK_LEGACY)

# Background processes
add_subdirectory(kpasswdserver)
add_subdirectory(kdontchangethehostname)
add_subdirectory(kglobalaccel)
add_subdirectory(knotify)
add_subdirectory(ktimezoned)

if(NOT WINCE)
add_subdirectory(kuiserver)
endif(NOT WINCE)
add_subdirectory(kwalletd)

if(QCA2_FOUND AND EXISTS ${KDE4_INCLUDE_INSTALL_DIR}/ksecretsservice/ksecretsserviceglobals.h)
  add_subdirectory(ksecretsserviced)
endif()

if ( UNIX )
   add_subdirectory(soliduiserver)
   add_subdirectory(solidautoeject)
   add_subdirectory(solid-device-automounter)
   add_subdirectory(solid-networkstatus)
endif ( UNIX )
if(NOT WIN32 AND SHAREDDESKTOPONTOLOGIES_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_PLUGIN_REDLANDBACKEND_FOUND AND SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)
  macro_optional_add_subdirectory(nepomuk)
endif(NOT WIN32 AND SHAREDDESKTOPONTOLOGIES_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY AND SOPRANO_PLUGIN_REDLANDBACKEND_FOUND AND SOPRANO_PLUGIN_VIRTUOSOBACKEND_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND)

# Plugins/components required for basic utility
if ( NOT WINCE )
    add_subdirectory(kcontrol)
endif ( NOT WINCE )
add_subdirectory(kioslave)
add_subdirectory(kurifilter-plugins)
add_subdirectory(phonon)
macro_optional_add_subdirectory(renamedlgplugins)

# UI Helper applications
add_subdirectory(drkonqi)
add_subdirectory(kcmshell)
add_subdirectory(kdebugdialog)
if ( ${QT_QT3SUPPORT_FOUND} )
   add_subdirectory(khelpcenter)
endif ( ${QT_QT3SUPPORT_FOUND} )
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
if ( NOT WINCE )
    add_subdirectory(plasma)
endif ( NOT WINCE )
add_subdirectory(solid-hardware)

# Platform-specific integration
add_subdirectory(platforms)

# Default settings, content and config
add_subdirectory(l10n)
add_subdirectory(localization)
if(NOT WINCE)
  add_subdirectory(kde-menu)
macro_optional_add_subdirectory(doc)
endif(NOT WINCE)

# Artwork
add_subdirectory(desktoptheme)
if ( NOT WINCE )
  add_subdirectory(pics)
endif ( NOT WINCE )

# KDE integration for attica
if(LIBATTICA_FOUND)
  macro_optional_add_subdirectory(attica)
endif(LIBATTICA_FOUND)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

########### install files ###############
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/kde4 DESTINATION ${BIN_INSTALL_DIR})
