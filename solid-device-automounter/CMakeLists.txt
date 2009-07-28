project(device_automounter)
find_package(KDE4)
include(KDE4Defaults)

include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_BINARY_DIR}/lib)

add_subdirectory(lib)
add_subdirectory(kcm)
add_subdirectory(kded)
