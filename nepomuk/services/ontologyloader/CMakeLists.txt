project(ontologyloader)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300103)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(ontologyloader_SRCS
  ontologyloader.cpp
  ontologymanagermodel.cpp
  graphretriever.cpp
  crappyinferencer.cpp
  )

qt4_add_dbus_adaptor(ontologyloader_SRCS
  ../../interfaces/org.kde.nepomuk.OntologyManager.xml
  ontologyloader.h
  Nepomuk::OntologyLoader)

kde4_add_plugin(nepomukontologyloader ${ontologyloader_SRCS})

target_link_libraries(nepomukontologyloader
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukontologyloader.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukontologyloader
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
