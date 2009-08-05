project(kioslave_nepomuksearch)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300200)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KDE4_KIO_INCLUDES}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomuk_SOURCE_DIR}/libnepomukquery
)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(kio_nepomuksearch_PART_SRCS
  kio_nepomuksearch.cpp
  searchfolder.cpp
  queryserviceclient.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/result.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/query.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/term.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/queryparser.cpp
  ${nepomuk_SOURCE_DIR}/libnepomukquery/dbusoperators.cpp
)

soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${nepomukontologies_SOURCE_DIR}/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${nepomukontologies_SOURCE_DIR}/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${nepomukontologies_SOURCE_DIR}/pimo.trig
  "PIMO"
  "Nepomuk::Vocabulary"
  "trig")

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.QueryService.xml
  PROPERTIES INCLUDE "querymetatype.h")

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.Query.xml
  PROPERTIES INCLUDE "result.h")

qt4_add_dbus_interface(kio_nepomuksearch_PART_SRCS
  ../../interfaces/org.kde.nepomuk.QueryService.xml
  queryserviceinterface)
qt4_add_dbus_interface(kio_nepomuksearch_PART_SRCS
  ../../interfaces/org.kde.nepomuk.Query.xml
  queryinterface)

kde4_add_plugin(kio_nepomuksearch
  ${kio_nepomuksearch_PART_SRCS})

target_link_libraries(kio_nepomuksearch
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  TARGETS kio_nepomuksearch
  DESTINATION ${PLUGIN_INSTALL_DIR})
install(
  FILES nepomuksearch.protocol
  DESTINATION  ${SERVICES_INSTALL_DIR})
