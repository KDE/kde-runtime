project(queryservice)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${NEPOMUK_INCLUDES}
  ${nepomuk_SOURCE_DIR}/libnepomukquery
)

set(queryservice_SRCS
  queryservice.cpp
  searchcore.cpp
  searchthread.cpp
  folder.cpp
  folderconnection.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/result.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/query.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/term.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/queryparser.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/dbusoperators.cpp
)

qt4_add_dbus_adaptor(queryservice_SRCS
  ../../interfaces/org.kde.nepomuk.Query.xml
  folderconnection.h
  Nepomuk::Search::FolderConnection)

kde4_add_plugin(nepomukqueryservice ${queryservice_SRCS})

target_link_libraries(nepomukqueryservice
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukqueryservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukqueryservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
