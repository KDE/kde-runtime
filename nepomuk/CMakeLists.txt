project(nepomuk)

# find dependancies
macro_optional_find_package(CLucene)
macro_log_feature(CLucene_FOUND
  "CLucene"
  "C++ search engine"
  "http://sourceforge.net/projects/clucene"
  FALSE 
  "0.9.19"
  "For building the Nepomuk Strigi backend. Recommended.")

# disable deprecated libnepomuk stuff for less compiler warnings
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
)

# needs to be included before everything else
add_subdirectory(ontologies)

# optional Strigi backend
# needs to be included before "services"
if(CLucene_FOUND AND SopranoIndex_FOUND)
  set(HAVE_STRIGI_SOPRANO_BACKEND "yes")
  macro_optional_add_subdirectory(strigibackend)
endif(CLucene_FOUND AND SopranoIndex_FOUND)

#add_subdirectory(libnepomukquery)
add_subdirectory(server)
add_subdirectory(kcm)
add_subdirectory(services)
add_subdirectory(servicestub)
add_subdirectory(kioslaves)
add_subdirectory(interfaces)

#add_subdirectory(test)
