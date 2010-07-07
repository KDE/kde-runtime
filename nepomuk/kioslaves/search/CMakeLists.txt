project(kioslave_nepomuksearch)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300200)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KDE4_KIO_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomuk_kio_slaves_SOURCE_DIR}/common
)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DDISABLE_NEPOMUK_LEGACY)

set(kio_nepomuksearch_PART_SRCS
  kio_nepomuksearch.cpp
  searchfolder.cpp
  ../common/resourcestat.cpp
)

soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/pimo/pimo.trig
  "PIMO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kio_nepomuksearch_PART_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nuao.trig
  "NUAO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_plugin(kio_nepomuksearch
  ${kio_nepomuksearch_PART_SRCS})

target_link_libraries(kio_nepomuksearch
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

kde4_add_executable(kio_nepomuksearch_userquery_helper userqueryhelper.cpp)
target_link_libraries(kio_nepomuksearch_userquery_helper
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  )

install(
  TARGETS kio_nepomuksearch
  DESTINATION ${PLUGIN_INSTALL_DIR})
install(
  TARGETS kio_nepomuksearch_userquery_helper
  DESTINATION ${KDE4_LIBEXEC_INSTALL_DIR})
install(
  FILES nepomuksearch.protocol
  DESTINATION  ${SERVICES_INSTALL_DIR})
install( FILES
  nepomuksearch_userquery_actions.desktop
  DESTINATION
  ${SERVICES_INSTALL_DIR}/ServiceMenus )

add_subdirectory(kdedmodule)
