project(nepomuk)

add_subdirectory(service-registry)
add_subdirectory(coreservices)
if(KDE4_BUILD_TESTS)
  add_subdirectory(tests)
endif(KDE4_BUILD_TESTS)
