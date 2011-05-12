project(libnepomuksync)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(KDE4Defaults)
include(SopranoAddOntology)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KDE4_KDECORE_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

set(nepomuksync_SRCS
    resourcemerger.cpp
    resourceidentifier.cpp
    resourceidentifier_p.cpp
    identificationsetgenerator.cpp
    syncresource.cpp
)

soprano_add_ontology(nepomuksync_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../../../ontologies/nrio.trig
  "NRIO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_library(nepomuksync SHARED ${nepomuksync_SRCS})

set_target_properties(nepomuksync PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

target_link_libraries(nepomuksync
  ${QT_QTCORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${KDE4_KIO_LIBS}
  kdecore
  )

install(TARGETS nepomuksync EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
# 
# install(FILES
#     resourceidentifier.h
#     resourcemerger.h
#     syncresource.h
#     nepomuksync_export.h
#     DESTINATION ${INCLUDE_INSTALL_DIR}/nepomuk COMPONENT Devel
# )
