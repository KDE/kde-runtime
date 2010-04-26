project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

#search packages used by KDE
find_package(KDE4 REQUIRED)
if ( NOT ONLY_KLEO )
find_package(Strigi REQUIRED)
# I don't see any package requiring it. please explain why this dependency
# is necessary (dirk)
#find_package(KdepimLibs REQUIRED)
endif ( NOT ONLY_KLEO )

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

if ( NOT ONLY_KLEO )
set(SOPRANO_MIN_VERSION "2.0.98")
macro_optional_find_package(Soprano)
macro_optional_find_package(Nepomuk)
macro_log_feature(Soprano_FOUND "Soprano" "Soprano Libraries" "kdesupport" FALSE "" "Required to build Nepomuk.")
macro_log_feature(Nepomuk_FOUND "Nepomuk" "Nepomuk Libraries" "kdelibs" FALSE "" "Required to build Nepomuk.")
endif ( NOT ONLY_KLEO )

check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE) # for off_t, in at least kio_trash and kio_smb

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

if ( NOT ONLY_KLEO )
add_subdirectory(cmake)
endif ( NOT ONLY_KLEO )
macro_optional_add_subdirectory(doc)
if ( NOT ONLY_KLEO )
add_subdirectory(kuiserver)
if ( NOT WIN32 )
  add_subdirectory(drkonqi)
endif ( NOT WIN32 )
add_subdirectory(khelpcenter)
add_subdirectory(kioexec)
add_subdirectory(kioslave)
add_subdirectory(knetattach)
add_subdirectory(kpasswdserver)
endif ( NOT ONLY_KLEO )
add_subdirectory(kdebugdialog)
if ( NOT ONLY_KLEO )
add_subdirectory(ktimezoned)
add_subdirectory(kcontrol)
add_subdirectory(kcmshell)
add_subdirectory(knewstuff)
if ( UNIX )
   add_subdirectory(kdeeject)
   add_subdirectory(kdesu)
   add_subdirectory(soliduiserver)
endif ( UNIX )
add_subdirectory(kurifilter-plugins)
add_subdirectory(l10n)
add_subdirectory(phonon)
add_subdirectory(pics)
add_subdirectory(kstyles)
add_subdirectory(solid-hardware)

# Command-line tools (e.g. for shell scripts)
add_subdirectory(kquitapp)
add_subdirectory(kioclient)
add_subdirectory(ktraderclient)
add_subdirectory(kreadconfig)
add_subdirectory(kmimetypefinder)
add_subdirectory(kiconfinder)
if(NOT MSVC)
  # conflicts with kfile library from kdelibs
  add_subdirectory(kfile)
endif(NOT MSVC)
if (Q_WS_X11)
  add_subdirectory(kstart)
endif (Q_WS_X11)
add_subdirectory(knotify)
add_subdirectory(kde-menu)
add_subdirectory( menu )
add_subdirectory( platforms )

if(Soprano_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY)
  macro_optional_add_subdirectory(nepomuk)
endif(Soprano_FOUND AND Nepomuk_FOUND AND STRIGI_STRIGIQTDBUSCLIENT_LIBRARY)

macro_optional_add_subdirectory(renamedlgplugins)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

########### install files ###############
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/kde4 DESTINATION ${BIN_INSTALL_DIR})
endif ( NOT ONLY_KLEO )
