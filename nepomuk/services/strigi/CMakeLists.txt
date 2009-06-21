project(nepomukstrigiservice)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

find_package(LibKNotificationItem-1 REQUIRED)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  ${nepomukstrigiservice_BUILD_DIR}
  ${LIBKNOTIFICATIONITEM-1_INCLUDE_DIR}
  )

set(strigiservice_SRCS
  strigiservice.cpp
  strigiserviceadaptor.cpp
  indexscheduler.cpp
  priority.cpp
  strigiserviceconfig.cpp
  eventmonitor.cpp
  systray.cpp
  statuswidget.cpp
  filesystemwatcher.cpp
  )

qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.Storage.xml nepomukstorageinterface)

kde4_add_ui_files(strigiservice_SRCS
  statuswidget.ui)

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
  ${LIBKNOTIFICATIONITEM-1_LIBRARIES}
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${STRIGI_STREAMS_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${KDE4_KUTILS_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  FILES nepomukstrigiservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  FILES nepomukstrigiservice.notifyrc
  DESTINATION ${DATA_INSTALL_DIR}/nepomukstrigiservice)

install(
  TARGETS nepomukstrigiservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
