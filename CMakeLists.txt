project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

#search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(Strigi REQUIRED)
find_package(KdepimLibs REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)
include (MacroOptionalFindPackage)
include(MacroOptionalAddSubdirectory)
include(CheckFunctionExists)
include(CheckIncludeFiles)

check_include_files(sys/wait.h HAVE_SYS_WAIT_H)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

add_subdirectory(cmake)
add_subdirectory(kuiserver)
if ( NOT WIN32 )
  add_subdirectory(drkonqi)
  add_subdirectory(khelpcenter)
endif ( NOT WIN32 )
add_subdirectory(kioexec)
add_subdirectory(kioslave)
add_subdirectory(knetattach)
add_subdirectory(kpasswdserver)
add_subdirectory(kdebugdialog)
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

# Command-line tools (e.g. for shell scripts)
add_subdirectory(kquitapp)
add_subdirectory(kioclient)
add_subdirectory(ktraderclient)
add_subdirectory(kreadconfig)
add_subdirectory(kmimetypefinder)
if(NOT MSVC)
  # conflicts with kfile library from kdelibs
  add_subdirectory(kfile)
endif(NOT MSVC)
if (Q_WS_X11)
  add_subdirectory(kstart)
endif (Q_WS_X11)
add_subdirectory(knotify)
add_subdirectory(kde-menu)
add_subdirectory(doc)

find_package(Soprano)
find_package(Nepomuk)

#Anybody who looks at this: the nepomuk subdir is disabled because it
#hasn't been ported to Soprano2 and the new nepomuk libs from kdelibs
#If you want to fix that please do so, else let it commented out
if(Soprano_FOUND AND Nepomuk_FOUND)
#   macro_optional_add_subdirectory(nepomuk)
endif(Soprano_FOUND AND Nepomuk_FOUND)

macro_optional_add_subdirectory(renamedlgplugins)
