project(nepomukstrigiservice)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  )

set(strigiservice_SRCS
  strigiservice.cpp
  strigiserviceadaptor.cpp
  indexscheduler.cpp
  priority.cpp
  config.cpp
  eventmonitor.cpp
  systray.cpp
  statuswidget.cpp
  )

kde4_add_ui_files(strigiservice_SRCS
  statuswidget.ui)

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
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
