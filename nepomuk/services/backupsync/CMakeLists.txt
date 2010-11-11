cmake_minimum_required (VERSION 2.6) 
project(nepomukbackupsync)

# disable deprecated libnepomuk stuff for less compiler warnings
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

add_subdirectory(lib)
add_subdirectory(service)
add_subdirectory(gui)

