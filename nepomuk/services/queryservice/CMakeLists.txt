project(queryservice)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${NEPOMUK_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

set(queryservice_SRCS
  queryservice.cpp
  searchcore.cpp
  searchthread.cpp
  folder.cpp
  folderconnection.cpp
  lib/result.cpp
  lib/query.cpp
  lib/term.cpp
  lib/queryparser.cpp
  lib/dbusopertors.cpp
)

qt4_add_dbus_adaptor(queryservice_SRCS
  ../interfaces/org.kde.nepomuk.Query.xml
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
