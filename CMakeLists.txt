project(KDEBASE_RUNTIME)


cmake_minimum_required(VERSION 2.8.6 FATAL_ERROR)
set(QT_MIN_VERSION "5.2.0")

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

################# set KDE specific information #################

find_package(ECM 0.0.8 REQUIRED NO_MODULE)

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(CheckIncludeFiles)
include(WriteBasicConfigVersionFile)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include(ECMOptionalAddSubdirectory)
include(ECMMarkNonGuiExecutable)
include(ECMInstallIcons)

find_package(Qt5Core REQUIRED NO_MODULE)
find_package(Qt5DBus REQUIRED NO_MODULE)
find_package(Qt5PrintSupport REQUIRED NO_MODULE)
find_package(Qt5Test REQUIRED NO_MODULE)
find_package(Qt5Svg REQUIRED NO_MODULE)
find_package(Qt5Quick REQUIRED NO_MODULE)
find_package(Qt5Widgets REQUIRED NO_MODULE)
find_package(Qt5X11Extras REQUIRED NO_MODULE)
find_package(Qt5Concurrent REQUIRED NO_MODULE)

#include(ConfigureChecks.cmake)

# Load CMake, Compiler and InstallDirs settings from KF5 and the following are already somewhat "done" tier1/tier2 libs from kdelibs:
find_package(KF5 CONFIG REQUIRED COMPONENTS
    IdleTime ItemModels WidgetsAddons WindowSystem Codecs Archive CoreAddons Solid ThreadWeaver
    Config Auth JS Wallet DBusAddons Sonnet Init GlobalAccel
    I18n GuiAddons Service ConfigWidgets ItemViews Notifications IconThemes Completion JobWidgets TextWidgets XmlGui Crash
    KCMUtils UnitConversion Bookmarks KIO Parts Kross Declarative NewStuff NotifyConfig Emoticons Sonnet KHtml KCMUtils Su)

find_package(KF5Plasma REQUIRED NO_MODULE)

# needed for set_package_properties macro and many projects depend on it stil
find_package(KF5KDE4Support REQUIRED NO_MODULE)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Background processes
# add_subdirectory(kpasswdserver)
add_subdirectory(kdontchangethehostname)
add_subdirectory(kglobalaccel)
# add_subdirectory(knotify)
add_subdirectory(ktimezoned)
add_subdirectory(kuiserver)

if ( UNIX )
   add_subdirectory(soliduiserver)
   add_subdirectory(solidautoeject)
#    add_subdirectory(solid-device-automounter)
   add_subdirectory(solid-networkstatus)
endif ( UNIX )

if(NepomukCore_FOUND)
#     macro_optional_add_subdirectory(nepomuk)
endif(NepomukCore_FOUND)

# Plugins/components required for basic utility
add_subdirectory(kcontrol)
add_subdirectory(kioslave)
add_subdirectory(kurifilter-plugins)
# add_subdirectory(phonon)
# macro_optional_add_subdirectory(renamedlgplugins)

# UI Helper applications
add_subdirectory(drkonqi)
add_subdirectory(kcmshell)
add_subdirectory(kdebugdialog)
add_subdirectory(khelpcenter)
add_subdirectory(knetattach)
add_subdirectory(knewstuff)
add_subdirectory(keditfiletype)
if ( UNIX )
   add_subdirectory(kdesu)
endif ( UNIX )

# Command-line tools (e.g. for shell scripts)
if ( UNIX )
#    add_subdirectory(kdeeject)
endif ( UNIX )
# add_subdirectory(kfile)
add_subdirectory(kioclient)
add_subdirectory(kioexec)
add_subdirectory(ktraderclient)
add_subdirectory(kreadconfig)
# add_subdirectory(kmimetypefinder)
if (Q_WS_X11)
  add_subdirectory(kstart)
endif (Q_WS_X11)
add_subdirectory(menu)
add_subdirectory(solid-hardware)

# Platform-specific integration
# add_subdirectory(platforms)

# Default settings, content and config
# add_subdirectory(l10n)
# add_subdirectory(localization)
# add_subdirectory(kde-menu)
# macro_optional_add_subdirectory(doc)

# Artwork
# add_subdirectory(desktoptheme)
# add_subdirectory(pics)

# KDE integration for attica
if(LIBATTICA_FOUND)
#   macro_optional_add_subdirectory(attica)
endif(LIBATTICA_FOUND)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
