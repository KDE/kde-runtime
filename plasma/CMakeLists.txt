find_package(KDE4 4.6.40 REQUIRED)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
${CMAKE_MODULE_PATH})

include_directories(/usr/include/qt4
                    /usr/include/qt4/QtCore
                    /usr/include/qt4/QtGui
                    /usr/include/qt4/QtScript
                    /usr/include/qt4/QtDeclarative
                    /usr/include/qt4/QtNetwork
                    /usr/include/qt4/QtUiTools
                    /usr/include/KDE)

set(KDE_MIN_VERSION "4.6.40")  # for the < 4.2 macro
include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

add_subdirectory(containments)
add_subdirectory(declarativeimports)
add_subdirectory(kpart)
add_subdirectory(remotewidgetshelper)
add_subdirectory(scriptengines)
add_subdirectory(tools)
