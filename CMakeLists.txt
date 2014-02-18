project(KDEBASE_RUNTIME)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(CTest)
include(CTestConfig.cmake)
include(CheckIncludeFiles)
include(CheckFunctionExists)

if(WINCE)
    #Needs to be set because otherwise he wouldn't find kde parts
    set(KDE4_DATA_DIR "${CMAKE_PREFIX_PATH}/share/apps")
    add_definitions(-DKDELIBS_STATIC_LIBS)
endif(WINCE)

#search packages used by KDE
find_package(KDE4 4.9.95 REQUIRED)
include(KDE4Defaults)

find_package(KDeclarative QUIET CONFIG)
set_package_properties(KDeclarative PROPERTIES DESCRIPTION "KDE Declarative (QML) support from kdelibs"
                       URL "http://www.kde.org"
                       TYPE REQUIRED
                       PURPOSE "Required by corebindingsplugin (org.kde.plasma.core)"
                      )

macro_optional_find_package(NepomukCore)
set_package_properties(NepomukCore PROPERTIES DESCRIPTION "Nepomuk Core Libraries"
                       URL "https://projects.kde.org/nepomuk-core"
                       TYPE OPTIONAL
                      )

macro_optional_find_package(SLP)
set_package_properties(SLP PROPERTIES DESCRIPTION "SLP (Service Location Protocol) implementation"
                       URL "http://www.openslp.org/"
                       TYPE OPTIONAL
                       PURPOSE "Provides SLP support in the network:/ kioslave."
                      )

find_package(LibAttica 0.1.4)
set_package_properties(LibAttica PROPERTIES DESCRIPTION "A library to access Open Collaboration Service providers"
                       URL "https://projects.kde.org/attica"
                       TYPE REQUIRED
                       PURPOSE "Attica is needed for the Get Hot New Stuff support"
                      )

macro_optional_find_package(QCA2 2.0.0)
set_package_properties(QCA2 PROPERTIES DESCRIPTION "Support for remote plasma widgets"
                       URL "http://delta.affinix.com/qca"
                       TYPE OPTIONAL
                      )

find_package(LibGcrypt 1.5.0 REQUIRED QUIET)
set_package_properties(LibGcrypt PROPERTIES DESCRIPTION "Libgcrypt is a general purpose cryptographic library based on the code from GnuPG."
                       URL "http://www.gnu.org/software/libgcrypt/"
                       TYPE REQUIRED
                       PURPOSE "kwalletd needs libgcrypt to perform PBKDF2-SHA512 hashing"
                      )
# Build options
option(KDERUNTIME_BUILD_NEPOMUK "Build the Nepomuk KCM and kioslaves" FALSE)

check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

# Build infrastructure
add_subdirectory(cmake)

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

if ( UNIX )
   add_subdirectory(soliduiserver)
   add_subdirectory(solidautoeject)
   add_subdirectory(solid-device-automounter)
   add_subdirectory(solid-networkstatus)
endif ( UNIX )

if(NepomukCore_FOUND AND KDERUNTIME_BUILD_NEPOMUK)
    macro_optional_add_subdirectory(nepomuk)
endif()

# Plugins/components required for basic utility
if ( NOT WINCE )
    add_subdirectory(kcontrol)
endif ( NOT WINCE )
add_subdirectory(kioslave)
add_subdirectory(kurifilter-plugins)
add_subdirectory(phonon)
add_subdirectory(kimgio)
macro_optional_add_subdirectory(renamedlgplugins)

# UI Helper applications
add_subdirectory(drkonqi)
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

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

########### install files ###############
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/kde4 DESTINATION ${BIN_INSTALL_DIR})

