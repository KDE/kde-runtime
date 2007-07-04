
include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
)

set( nepomukdaemon_BIN_SRCS
  main.cpp
)

kde4_automoc(${nepomukdaemon_BIN_SRCS})

kde4_add_executable(nepomukdaemon ${nepomukdaemon_BIN_SRCS})

target_link_libraries(nepomukdaemon
  nepomukregcore 
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
)

configure_file(org.semanticdesktop.nepomuk.ServiceRegistry.service.cmake 
  ${CMAKE_CURRENT_BINARY_DIR}/org.semanticdesktop.nepomuk.ServiceRegistry.service)

install(TARGETS nepomukdaemon DESTINATION ${BIN_INSTALL_DIR} )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.semanticdesktop.nepomuk.ServiceRegistry.service
  DESTINATION ${DBUS_SERVICES_DIR})
