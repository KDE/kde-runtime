include(SopranoAddOntology)

set(nepomukactivitiesservice_SRCS
   nepomukactivitiesservice.cpp
   )

qt4_add_dbus_adaptor(nepomukactivitiesservice_SRCS
   org.kde.nepomuk.ActivitiesService.xml
   nepomukactivitiesservice.h
   NepomukActivitiesService
   )

soprano_add_ontology(
   nepomukactivitiesservice_SRCS
   "${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig"
   "nie"
   "Ontologies"
   "trig"
   )

kde4_add_plugin(
   nepomukactivitiesservice ${nepomukactivitiesservice_SRCS}
   )

target_link_libraries(nepomukactivitiesservice
   ${SOPRANO_CLIENT_LIBRARIES}
   ${SOPRANO_LIBRARIES}
   ${QT_QTCORE_LIBRARY}
   ${QT_QTDBUS_LIBRARY}
   ${KDE4_KDECORE_LIBS}
   ${NEPOMUK_LIBRARIES}
   ${NEPOMUK_QUERY_LIBRARIES}
   )

install(
   FILES nepomukactivitiesservice.desktop
   DESTINATION ${SERVICES_INSTALL_DIR}
   )

install(
   TARGETS nepomukactivitiesservice
   DESTINATION ${PLUGIN_INSTALL_DIR}
   )

# install(FILES
#    org.kde.nepomuk.ActivitiesService.xml
#    DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
#    )
