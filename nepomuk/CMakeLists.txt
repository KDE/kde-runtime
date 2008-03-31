project(nepomuk)

if(SopranoIndex_FOUND)
  set(HAVE_SOPRANO_INDEX "yes")
  include_directories(${STRIGI_INCLUDE_DIR})
endif(SopranoIndex_FOUND)

# optional Strigi backend
# -----------------------------
find_package(CLucene)
if(CLucene_FOUND AND SopranoIndex_FOUND)
  set(HAVE_STRIGI_SOPRANO_BACKEND "yes")
  macro_optional_add_subdirectory(strigibackend)
endif(CLucene_FOUND AND SopranoIndex_FOUND)
# -----------------------------

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(kcm)
add_subdirectory(services)
add_subdirectory(servicestub)
add_subdirectory(interfaces)

#add_subdirectory(test)
