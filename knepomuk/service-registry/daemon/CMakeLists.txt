project(knepdaemon)

include_directories(
  ${knepregistry_SOURCE_DIR}
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
)

set( knepdaemon_BIN_SRCS
  main.cpp
)

kde4_automoc(${knepdaemon_BIN_SRCS})

kde4_add_executable(knepdaemon ${knepdaemon_BIN_SRCS})

target_link_libraries(knepdaemon
  knepregcore 
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
)

configure_file(org.semanticdesktop.nepomuk.ServiceRegistry.service.cmake 
  ${CMAKE_CURRENT_BINARY_DIR}/org.semanticdesktop.nepomuk.ServiceRegistry.service)

install(TARGETS knepdaemon DESTINATION ${BIN_INSTALL_DIR} )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.semanticdesktop.nepomuk.ServiceRegistry.service
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
