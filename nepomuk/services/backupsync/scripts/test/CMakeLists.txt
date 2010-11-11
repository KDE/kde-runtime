project(nepomuk-test)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(SopranoAddOntology)
include (KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set( SRCS
  main.cpp
)

#--------- Ontologies -------#
soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nao.trig
  "NAO"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nco.trig
  "NCO"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nmm.trig
  "NMM"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_executable(nepomuk-test ${SRCS})


target_link_libraries( nepomuk-test
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(TARGETS nepomuk-test
        DESTINATION ${BIN_INSTALL_DIR})
