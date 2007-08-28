project(KDEBASE_PHONON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules )

#search packages used by KDE
find_package(KDE4 REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)
include (MacroOptionalFindPackage)
include(MacroOptionalAddSubdirectory)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

macro_optional_find_package(Xine)
if(XINE_FOUND)
  macro_optional_add_subdirectory(xine)
endif(XINE_FOUND)
