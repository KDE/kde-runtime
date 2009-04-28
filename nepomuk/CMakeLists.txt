project(nepomuk)

# optional Strigi backend
# -----------------------------
macro_optional_find_package(CLucene)
macro_log_feature(CLucene_FOUND "CLucene" "C++ search engine" "http://sourceforge.net/projects/clucene" FALSE "0.9.19" "For building the Nepomuk Strigi backend. Recommended.")

if(CLucene_FOUND AND SopranoIndex_FOUND)
  set(HAVE_STRIGI_SOPRANO_BACKEND "yes")
  macro_optional_add_subdirectory(strigibackend)
endif(CLucene_FOUND AND SopranoIndex_FOUND)
# -----------------------------

if(SopranoIndex_FOUND)
  set(HAVE_SOPRANO_INDEX "yes")
  include_directories(${STRIGI_INCLUDE_DIR})
endif(SopranoIndex_FOUND)

include_directories(${SOPRANO_INCLUDE_DIR})

add_subdirectory(libnepomukquery)
add_subdirectory(ontologies)
add_subdirectory(server)
add_subdirectory(kcm)
add_subdirectory(services)
add_subdirectory(servicestub)
add_subdirectory(kioslaves)
add_subdirectory(interfaces)

if (NOT WIN32)
  add_subdirectory(doc)
endif (NOT WIN32)

#add_subdirectory(test)
