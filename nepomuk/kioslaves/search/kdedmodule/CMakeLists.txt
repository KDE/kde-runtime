include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomuk_kio_slaves_SOURCE_DIR}/common
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300200)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DDISABLE_NEPOMUK_LEGACY)

set(nepomuksearchmodule_SRCS
  nepomuksearchmodule.cpp
  searchurllistener.cpp
  ../../common/timelinetools.cpp
)

soprano_add_ontology(nepomuksearchmodule_SRCS ${nepomuk_ontologies_SOURCE_DIR}/kext.trig "KExt" "Nepomuk::Vocabulary" "trig")

set_source_files_properties(
  ${nepomuk_kio_slaves_SOURCE_DIR}/../interfaces/org.kde.nepomuk.Query.xml
  PROPERTIES INCLUDE "nepomuk/result.h")
set_source_files_properties(
  ${nepomuk_kio_slaves_SOURCE_DIR}/../interfaces/org.kde.nepomuk.QueryService.xml
  PROPERTIES INCLUDE "dbusoperators_p.h")

qt4_add_dbus_interface(nepomuksearchmodule_SRCS
  ${nepomuk_kio_slaves_SOURCE_DIR}/../interfaces/org.kde.nepomuk.QueryService.xml
  queryserviceinterface)
qt4_add_dbus_interface(nepomuksearchmodule_SRCS
  ${nepomuk_kio_slaves_SOURCE_DIR}/../interfaces/org.kde.nepomuk.Query.xml
  queryinterface)

kde4_add_plugin(kded_nepomuksearchmodule ${nepomuksearchmodule_SRCS})

target_link_libraries(kded_nepomuksearchmodule 
  nepomukcommon
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
)

install(TARGETS kded_nepomuksearchmodule DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES nepomuksearchmodule.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
