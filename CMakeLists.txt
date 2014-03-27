project(KDEBASE_RUNTIME)


cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
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

include(ECMMarkAsTest)
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


# Load CMake, Compiler and InstallDirs settings from KF5 and the following are already somewhat "done" tier1/tier2 libs from kdelibs:
find_package(KF5 CONFIG REQUIRED COMPONENTS
    IdleTime ItemModels WidgetsAddons WindowSystem Codecs Archive CoreAddons Solid ThreadWeaver
    Config Auth JS Wallet DBusAddons Sonnet Init GlobalAccel
    I18n GuiAddons Service ConfigWidgets ItemViews Notifications IconThemes Completion JobWidgets TextWidgets XmlGui Crash
    KCMUtils UnitConversion Bookmarks KIO Parts Kross Declarative NewStuff NotifyConfig Emoticons Sonnet KHtml KCMUtils Su Plasma DNSSD KDE4Support)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

#directories to split
add_subdirectory(kde-cli-tools)
add_subdirectory(kioslave)
add_subdirectory(kioslave-extra)
add_subdirectory(plasma-workspace)
add_subdirectory(plasma-desktop)
add_subdirectory(khelpcenter)

# Background processes
add_subdirectory(kglobalaccel)
add_subdirectory(knotify)

if ( UNIX )
   add_subdirectory(soliduiserver)
   add_subdirectory(solid-device-automounter)
   add_subdirectory(solid-networkstatus)
endif ()

# Plugins/components required for basic utility
add_subdirectory(kcontrol)
add_subdirectory(kurifilter-plugins)

# UI Helper applications
add_subdirectory(drkonqi)
add_subdirectory(kdebugdialog)
add_subdirectory(knewstuff)
if ( UNIX )
   add_subdirectory(kdesu)
endif ()

# Command-line tools (e.g. for shell scripts)
add_subdirectory(kfile)
add_subdirectory(kreadconfig)
add_subdirectory(solid-hardware)

# Platform-specific integration
if(WIN32)
    add_subdirectory(kwindowsaddons)
endif()

# Default settings, content and config
add_subdirectory(l10n)
add_subdirectory(localization)
add_subdirectory(kde-menu)
add_subdirectory(doc)

# KDE integration for attica
if(LIBATTICA_FOUND)
    add_subdirectory(attica)
endif()

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
