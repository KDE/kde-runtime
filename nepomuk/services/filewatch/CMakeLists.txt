project(nepomukfilewatch)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300101)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../fileindexer
  ${CMAKE_CURRENT_SOURCE_DIR}/../storage/lib/
  )

set(SRCS
  nepomukfilewatch.cpp
  metadatamover.cpp
  updaterequest.cpp
  invalidfileresourcecleaner.cpp
  ../fileindexer/fileindexerconfig.cpp
  removabledeviceindexnotification.cpp
  )

qt4_add_dbus_interface(SRCS ../../interfaces/org.kde.nepomuk.FileIndexer.xml fileindexerinterface)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(SRCS
    ${SRCS}
    kinotify.cpp
)
add_definitions(-DBUILD_KINOTIFY)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

kde4_add_plugin(nepomukfilewatch ${SRCS})

target_link_libraries(nepomukfilewatch
  nepomukcommon
  nepomukdatamanagement
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  )

install(
  FILES nepomukfilewatch.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukfilewatch
  DESTINATION ${PLUGIN_INSTALL_DIR})
install(
  FILES nepomukfilewatch.notifyrc
  DESTINATION ${DATA_INSTALL_DIR}/nepomukfilewatch)

add_subdirectory(test)
