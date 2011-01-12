
add_subdirectory( qtextracomponents )
add_subdirectory( test )

FILE(GLOB plasmacomponents plasmacomponents/*.qml)
install( FILES ${plasmacomponents} DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/plasma/components )

FILE(GLOB plasmacomponentsqmldir plasmacomponents/qmldir)
install( FILES ${plasmacomponentsqmldir} DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/plasma/components )