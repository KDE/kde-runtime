project(nepomukstrigiservice)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  ${nepomukstrigiservice_BUILD_DIR}
  )

set(strigiservice_SRCS
  strigiservice.cpp
  indexscheduler.cpp
  strigiserviceconfig.cpp
  eventmonitor.cpp
  nepomukindexwriter.cpp
  nepomukindexfeeder.cpp
  util.cpp
  nepomukindexer.cpp
  )

soprano_add_ontology(strigiservice_SRCS ${nepomuk_ontologies_SOURCE_DIR}/kext.trig "KExt" "Nepomuk::Vocabulary" "trig")

qt4_add_dbus_adaptor(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.Strigi.xml strigiservice.h Nepomuk::StrigiService )

qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.RemovableStorage.xml removablestorageserviceinterface)
qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.FileWatch.xml filewatchserviceinterface)

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
  nepomukcommon
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${STRIGI_STREAMS_LIBRARY}
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
# -----------------------------
