project(nepomukcontroller)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${CMAKE_CURRENT_BUILD_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../kcm
  )

set(controller_SRCS
  main.cpp
  systray.cpp
  ../kcm/statuswidget.cpp
  )

qt4_add_dbus_interface(controller_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.FileIndexer.xml fileindexerinterface)
qt4_add_dbus_interface(controller_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.ServiceControl.xml servicecontrol)

kde4_add_ui_files(controller_SRCS ../kcm/statuswidget.ui)

kde4_add_app_icon(controller_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/nepomuk.png")

kde4_add_executable(nepomukcontroller ${controller_SRCS})

target_link_libraries(nepomukcontroller
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
  )

install(
  PROGRAMS nepomukcontroller.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR})
install(
  PROGRAMS nepomukcontroller.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR})
install(
  TARGETS nepomukcontroller
  ${INSTALL_TARGETS_DEFAULT_ARGS})
