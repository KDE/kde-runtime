project(queryservice)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300104)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDES}
)

set(queryservice_SRCS
  queryservice.cpp
  searchthread.cpp
  folder.cpp
  folderconnection.cpp
)

qt4_add_dbus_adaptor(queryservice_SRCS
  ../../interfaces/org.kde.nepomuk.Query.xml
  folderconnection.h
  Nepomuk::Query::FolderConnection)

kde4_add_plugin(nepomukqueryservice ${queryservice_SRCS})

target_link_libraries(nepomukqueryservice
  nepomukcommon
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  )

install(
  FILES nepomukqueryservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukqueryservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
