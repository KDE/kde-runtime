project(fileindexerservice)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ../storage/lib/
  )

set(nepomukfileindexer_SRCS
  fileindexer.cpp
  indexscheduler.cpp
  indexcleaner.cpp
  fileindexerconfig.cpp
  eventmonitor.cpp
  util.cpp
  nepomukindexer.cpp
  )

qt4_add_dbus_adaptor(nepomukfileindexer_SRCS ../../interfaces/org.kde.nepomuk.FileIndexer.xml fileindexer.h Nepomuk::FileIndexer )

qt4_add_dbus_interface(nepomukfileindexer_SRCS ../../interfaces/org.kde.nepomuk.FileWatch.xml filewatchserviceinterface)

kde4_add_plugin(nepomukfileindexer ${nepomukfileindexer_SRCS})

target_link_libraries(nepomukfileindexer
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
  FILES nepomukfileindexer.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  FILES nepomukfileindexer.notifyrc
  DESTINATION ${DATA_INSTALL_DIR}/fileindexerservice)

install(
  TARGETS nepomukfileindexer
  DESTINATION ${PLUGIN_INSTALL_DIR})

add_subdirectory(indexer)
# -----------------------------
