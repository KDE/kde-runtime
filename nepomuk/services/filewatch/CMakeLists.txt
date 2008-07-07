project(nepomukfilewatch)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${CMAKE_SOURCE_DIR}
  ${SOPRANO_INCLUDE_DIR}
  ${nepomukfilewatch_BINARY_DIR}
  )

set(SRCS
  nepomukfilewatch.cpp
  )

kde4_add_plugin(nepomukfilewatch ${SRCS})

target_link_libraries(nepomukfilewatch
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukfilewatch.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukfilewatch
  DESTINATION ${PLUGIN_INSTALL_DIR})
