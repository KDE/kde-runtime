include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(kcms)
add_subdirectory(knetattach)

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )
