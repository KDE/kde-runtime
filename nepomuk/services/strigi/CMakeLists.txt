project(nepomukstrigiservice)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ../storage/lib/
  )

set(strigiservice_SRCS
  strigiservice.cpp
  indexscheduler.cpp
  strigiserviceconfig.cpp
  eventmonitor.cpp
  util.cpp
  nepomukindexer.cpp
  )

qt4_add_dbus_adaptor(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.Strigi.xml strigiservice.h Nepomuk::StrigiService )

qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.FileWatch.xml filewatchserviceinterface)

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
  nepomukcommon
  nepomukdatamanagement
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${KDE4_KIDLETIME_LIBS}
  ${NEPOMUK_QUERY_LIBRARIES}
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

add_subdirectory(indexer)
# -----------------------------
