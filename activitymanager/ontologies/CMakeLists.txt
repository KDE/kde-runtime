project(nepomuk_ontologies)

configure_file(kamdrso.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/kamdrso.ontology)

install(FILES
  kamdrso.trig
  ${CMAKE_CURRENT_BINARY_DIR}/kamdrso.ontology
DESTINATION ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)
