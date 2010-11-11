project(nepomuk_ontologies)

set(ONTO_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)
configure_file(kuvo.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/kuvo.ontology)
install(FILES kuvo.trig ${CMAKE_CURRENT_BINARY_DIR}/kuvo.ontology DESTINATION ${ONTO_INSTALL_DIR})

configure_file(nrio.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/nrio.ontology)
install(FILES nrio.trig ${CMAKE_CURRENT_BINARY_DIR}/nrio.ontology DESTINATION ${ONTO_INSTALL_DIR})
