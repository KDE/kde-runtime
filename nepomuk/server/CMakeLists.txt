project(nepomukserver)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300001)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomukserver_BINARY_DIR}
  )

# NepomukServer
# -----------------------------

set(nepomukserver_SRCS
  nepomukserver.cpp
  servicemanager.cpp
  servicecontroller.cpp
  processcontrol.cpp
  main.cpp
  )

# Server configuration
kde4_add_kcfg_files(nepomukserver_SRCS nepomukserversettings.kcfgc)

qt4_add_dbus_adaptor(nepomukserver_SRCS
  ../interfaces/org.kde.NepomukServer.xml
  nepomukserver.h
  Nepomuk::Server)
qt4_add_dbus_adaptor(nepomukserver_SRCS
  ../interfaces/org.kde.nepomuk.ServiceManager.xml
  servicemanager.h
  Nepomuk::ServiceManager)

qt4_add_dbus_interface(nepomukserver_SRCS
  ../interfaces/org.kde.nepomuk.ServiceControl.xml
  servicecontrolinterface)

kde4_add_kdeinit_executable(nepomukserver ${nepomukserver_SRCS})

if (Q_WS_MAC)
    set_target_properties(nepomukserver PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
    set_target_properties(nepomukserver PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.nepomukserver")
    set_target_properties(nepomukserver PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Nepomuk Server")
endif (Q_WS_MAC)


target_link_libraries(kdeinit_nepomukserver
  ${KDE4_KDEUI_LIBS}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${SOPRANO_CLIENT_LIBRARIES}
  )

target_link_libraries(nepomukserver
  kdeinit_nepomukserver
)

install(
  FILES nepomukservice.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR})

install(
  FILES nepomukserver.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR})

install(
  TARGETS kdeinit_nepomukserver
  DESTINATION ${LIB_INSTALL_DIR})

install(
  TARGETS nepomukserver
  ${INSTALL_TARGETS_DEFAULT_ARGS})
# -----------------------------
