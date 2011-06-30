project(nepomukindexer)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(KDE4Defaults)
include(SopranoAddOntology)

# Check if Strigi has the FileInputStream::open method which was introduced between versions 0.7.2 and 0.7.3
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${STRIGI_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${STRIGI_STREAMS_LIBRARY})
CHECK_CXX_SOURCE_COMPILES(
  "#include <strigi/fileinputstream.h>\nint main(int ac,char* av[]) { Strigi::FileInputStream::open(\"dummy\"); }"
  STRIGI_HAS_FILEINPUTSTREAM_OPEN
)
if(${STRIGI_HAS_FILEINPUTSTREAM_OPEN})
  add_definitions(-DSTRIGI_HAS_FILEINPUTSTREAM_OPEN)
endif(${STRIGI_HAS_FILEINPUTSTREAM_OPEN})

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${LIBSTREAMS_INCLUDE_DIRS}
  ${LIBSTREAMANALYZER_INCLUDE_DIRS}
  ../../storage/lib/
  )

set(indexer_SRCS
  main.cpp
  indexer.cpp
  nepomukindexwriter.cpp
  nepomukindexfeeder.cpp
  ../util.cpp
  ../../../servicestub/priority.cpp
  )

soprano_add_ontology(indexer_SRCS ${nepomuk_ontologies_SOURCE_DIR}/kext.trig "KExt" "Nepomuk::Vocabulary" "trig")

kde4_add_executable( nepomukindexer ${indexer_SRCS} )

target_link_libraries( nepomukindexer
  nepomukcommon
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${STRIGI_STREAMS_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  nepomukdatamanagement
  )

install(
  TARGETS nepomukindexer
  DESTINATION ${BIN_INSTALL_DIR} )
# -----------------------------
