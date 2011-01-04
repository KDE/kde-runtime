project(nepomuk_ontologies)

configure_file(kuvo.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/kuvo.ontology)
configure_file(nrio.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/nrio.ontology)
configure_file(kext.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/kext.ontology)

install(FILES
  kuvo.trig
  ${CMAKE_CURRENT_BINARY_DIR}/kuvo.ontology
  nrio.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nrio.ontology
  kext.trig
  ${CMAKE_CURRENT_BINARY_DIR}/kext.ontology 
DESTINATION ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)
