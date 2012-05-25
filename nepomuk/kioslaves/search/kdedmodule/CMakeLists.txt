include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${nepomuk_kio_slaves_SOURCE_DIR}/common
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300200)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DDISABLE_NEPOMUK_LEGACY)

set(nepomuksearchmodule_SRCS
  nepomuksearchmodule.cpp
  searchurllistener.cpp
  ../../common/timelinetools.cpp
)

soprano_add_ontology(nepomuksearchmodule_SRCS ${NEPOMUK_CORE_ONTOLOGY_DIR}/kde/kext.trig "KExt" "Nepomuk2::Vocabulary" "trig")

set_source_files_properties(
  ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.Query.xml
  PROPERTIES INCLUDE "nepomuk2/result.h")

set_source_files_properties(
  ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.QueryService.xml
  PROPERTIES INCLUDE "dbusoperators_p.h")

qt4_add_dbus_interface(nepomuksearchmodule_SRCS
  ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.QueryService.xml
  queryserviceinterface)
qt4_add_dbus_interface(nepomuksearchmodule_SRCS
  ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.Query.xml
  queryinterface)

kde4_add_plugin(kded_nepomuksearchmodule ${nepomuksearchmodule_SRCS})

target_link_libraries(kded_nepomuksearchmodule
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
)

install(TARGETS kded_nepomuksearchmodule DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES nepomuksearchmodule.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
