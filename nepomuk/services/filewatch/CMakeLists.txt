project(nepomukfilewatch)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300101)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../strigi
  )

set(SRCS
  nepomukfilewatch.cpp
  metadatamover.cpp
  updaterequest.cpp
  invalidfileresourcecleaner.cpp
  ../strigi/strigiserviceconfig.cpp
  )

qt4_add_dbus_interface(SRCS ../../interfaces/org.kde.nepomuk.Strigi.xml strigiserviceinterface)
qt4_add_dbus_interface(SRCS ../../interfaces/org.kde.nepomuk.RemovableStorage.xml removablestorageserviceinterface)

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
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  )

install(
  FILES nepomukfilewatch.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukfilewatch
  DESTINATION ${PLUGIN_INSTALL_DIR})
