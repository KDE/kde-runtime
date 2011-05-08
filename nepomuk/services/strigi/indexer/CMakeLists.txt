project(nepomukstrigiindexer)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(KDE4Defaults)
include(SopranoAddOntology)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  )

set(indexer_SRCS
  main.cpp
  indexer.cpp
  nepomukindexwriter.cpp
  nepomukindexfeeder.cpp
  ../util.cpp
  )

soprano_add_ontology(indexer_SRCS /home/vishesh/kde/src/kde-runtime/nepomuk/ontologies/kext.trig "KExt" "Nepomuk::Vocabulary" "trig")

kde4_add_executable( nepomukstrigiindexer ${indexer_SRCS} )

target_link_libraries( nepomukstrigiindexer
  nepomukcommon
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${STRIGI_STREAMS_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  TARGETS nepomukstrigiindexer
  DESTINATION ${BIN_INSTALL_DIR} )
# -----------------------------
