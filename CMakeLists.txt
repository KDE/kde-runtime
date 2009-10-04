project(KDEBASE_RUNTIME)

# Used e.g. in KDE4RuntimeConfig.cmake, Alex
set(KDE4RUNTIME_VERSION_MAJOR 4)
set(KDE4RUNTIME_VERSION_MINOR 3)
set(KDE4RUNTIME_VERSION_PATCH 61)
set(KDE4RUNTIME_VERSION ${KDE4RUNTIME_VERSION_MAJOR}.${KDE4RUNTIME_VERSION_MINOR}.${KDE4RUNTIME_VERSION_PATCH} )

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#search packages used by KDE
set(KDE_MIN_VERSION "4.3.60")  # for the < 4.2 macro
find_package(KDE4 4.3.60 REQUIRED)
find_package(Strigi REQUIRED)
# I don't see any package requiring it. please explain why this dependency
# is necessary (dirk)
#find_package(KdepimLibs REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

set(SOPRANO_MIN_VERSION "2.2.69")
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


# make the libraries installed from kdebase/runtime available to other projects
# by creating and installing a KDE4RuntimeConfig.cmake file, which will
# be searched and found by kdelibs/cmake/modules/FindKDE4Runtime.cmake. Alex

# now create the KDE4RuntimeConfig.cmake file, which will be loaded by
# kdelibs/cmake/modules/FindKDE4Runtime.cmake and which has to contain all information
# about the libraries installed from kdebase/runtime/libs/ anybody would like to have. Alex

# we need the absolute directories where stuff will be installed too
# but since the variables which contain the destinations can be relative
# or absolute paths, we need this macro to make them all absoulte, Alex
macro(MAKE_INSTALL_PATH_ABSOLUTE out in)
   if (IS_ABSOLUTE "${in}")    # IS_ABSOLUTE is new since cmake 2.4.8
      set(${out} "${in}")
   else (IS_ABSOLUTE "${in}")
      set(${out} "\${KDE4RUNTIME_INSTALL_DIR}/${in}")
   endif (IS_ABSOLUTE "${in}")
endmacro(MAKE_INSTALL_PATH_ABSOLUTE out in)

make_install_path_absolute(KDE4RUNTIME_LIB_DIR     ${LIB_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_LIBEXEC_DIR ${LIBEXEC_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_INCLUDE_DIR ${INCLUDE_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_BIN_DIR     ${BIN_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_SBIN_DIR    ${SBIN_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_DATA_DIR    ${DATA_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_HTML_DIR    ${HTML_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_CONFIG_DIR  ${CONFIG_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_ICON_DIR    ${ICON_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_KCFG_DIR    ${KCFG_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_LOCALE_DIR  ${LOCALE_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_MIME_DIR    ${MIME_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_SOUND_DIR   ${SOUND_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_TEMPLATES_DIR    ${TEMPLATES_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_WALLPAPER_DIR    ${WALLPAPER_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_KCONF_UPDATE_DIR    ${KCONF_UPDATE_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_AUTOSTART_DIR    ${AUTOSTART_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_XDG_APPS_DIR        ${XDG_APPS_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_XDG_DIRECTORY_DIR   ${XDG_DIRECTORY_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_SYSCONF_DIR ${SYSCONF_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_MAN_DIR     ${MAN_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_INFO_DIR    ${INFO_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_DBUS_INTERFACES_DIR ${DBUS_INTERFACES_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_DBUS_SERVICES_DIR   ${DBUS_SERVICES_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_SERVICES_DIR ${SERVICES_INSTALL_DIR})
make_install_path_absolute(KDE4RUNTIME_SERVICETYPES_DIR ${SERVICETYPES_INSTALL_DIR})

set(KDE4RUNTIME_TARGET_PREFIX KDE4Runtime__)

configure_file(KDE4RuntimeConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/KDE4RuntimeConfig.cmake" @ONLY)

# this file will be installed too and will be used by cmake when searching for the Config.cmake file to check the version of kdepimlibs, Alex
macro_write_basic_cmake_version_file(${CMAKE_CURRENT_BINARY_DIR}/KDE4RuntimeConfigVersion.cmake
                                     ${KDE4RUNTIME_VERSION_MAJOR} ${KDE4RUNTIME_VERSION_MINOR} ${KDE4RUNTIME_VERSION_PATCH})

set(_KDE4RuntimeConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/KDE4Runtime-${KDE4RUNTIME_VERSION}/cmake)
# places where find_package() looks for FooConfig.cmake files:
# CMake >= 2.6.0 looks in lib/Foo*/cmake/, CMake >= 2.6.3 also looks in
# lib/cmake/Foo*/, which packagers prefer. So they can set the KDE4_USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR
# option to have kdepimlibs install its Config file there. Alex
if(KDE4_USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
   set(_KDE4RuntimeConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/KDE4Runtime-${KDE4RUNTIME_VERSION} )
endif(KDE4_USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KDE4RuntimeConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/KDE4RuntimeConfigVersion.cmake
        DESTINATION ${_KDE4RuntimeConfig_INSTALL_DIR})

# install(EXPORT kderuntimeLibraryTargets
#         NAMESPACE ${KDE4RUNTIME_TARGET_PREFIX}
#         DESTINATION ${_KDE4RuntimeConfig_INSTALL_DIR}
#         FILE KDE4RuntimeLibraryTargets.cmake )
