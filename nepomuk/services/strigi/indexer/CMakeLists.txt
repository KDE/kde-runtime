project(nepomukindexer)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(KDE4Defaults)
include(SopranoAddOntology)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${LIBSTREAMS_INCLUDE_DIRS}
  ${LIBSTREAMANALYZER_INCLUDE_DIRS}
  )

set(indexer_SRCS
  main.cpp
  indexer.cpp
  nepomukindexwriter.cpp
  nepomukindexfeeder.cpp
  ../util.cpp
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
  )

install(
  TARGETS nepomukindexer
  DESTINATION ${BIN_INSTALL_DIR} )
# -----------------------------
