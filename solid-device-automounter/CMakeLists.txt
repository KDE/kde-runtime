project(device_automounter)
find_package(KDE4)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})

add_subdirectory(kcm)
add_subdirectory(kded)
