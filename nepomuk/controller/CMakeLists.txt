project(nepomukcontroller)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BUILD_DIR}
  )

set(controller_SRCS
  main.cpp
  systray.cpp
  )

qt4_add_dbus_interface(controller_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.nepomuk.Strigi.xml strigiserviceinterface)
qt4_add_dbus_interface(controller_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/org.kde.nepomuk.ServiceControl.xml servicecontrol)

kde4_add_executable(nepomukcontroller ${controller_SRCS})

target_link_libraries(nepomukcontroller
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  FILES nepomukcontroller.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR})

install(
  TARGETS nepomukcontroller
  ${INSTALL_TARGETS_DEFAULT_ARGS})
# -----------------------------
