project(nepomuk)

# disable deprecated libnepomuk stuff for less compiler warnings
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  ${nepomuk_SOURCE_DIR}/common
  ${nepomuk_BINARY_DIR}/common
)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(kcm)
add_subdirectory(services)
add_subdirectory(servicestub)
add_subdirectory(kioslaves)
add_subdirectory(interfaces)
add_subdirectory(ontologies)
add_subdirectory(removed-services)

#add_subdirectory(test)
