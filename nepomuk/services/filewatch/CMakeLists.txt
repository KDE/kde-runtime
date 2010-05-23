project(nepomukfilewatch)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300101)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(SRCS
  nepomukfilewatch.cpp
  metadatamover.cpp
  updaterequest.cpp
  )

soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_plugin(nepomukfilewatch ${SRCS})

target_link_libraries(nepomukfilewatch
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  )

install(
  FILES nepomukfilewatch.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukfilewatch
  DESTINATION ${PLUGIN_INSTALL_DIR})
