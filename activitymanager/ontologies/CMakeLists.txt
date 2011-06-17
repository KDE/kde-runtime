project(nepomuk_ontologies)

configure_file(krso.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/krso.ontology)

install(FILES
  krso.trig
  ${CMAKE_CURRENT_BINARY_DIR}/krso.ontology
DESTINATION ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)
