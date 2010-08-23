project(nepomukremovablestorageservice)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(SRCS
  removablestorageservice.cpp
  )

qt4_add_dbus_interface(SRCS ../../interfaces/org.kde.nepomuk.Strigi.xml strigiserviceinterface)
qt4_add_dbus_interface(SRCS ../../interfaces/org.kde.nepomuk.FileWatch.xml filewatchserviceinterface)

kde4_add_plugin(nepomukremovablestorageservice ${SRCS})

target_link_libraries(nepomukremovablestorageservice
  nepomukcommon
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${KDE4_SOLID_LIBS}
  )

install(
  FILES nepomukremovablestorageservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukremovablestorageservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
