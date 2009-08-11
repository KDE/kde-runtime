add_subdirectory( test )

set(kded_klockingserver_SRCS klockingserver.cpp )

qt4_add_dbus_adaptor(kded_klockingserver_SRCS org.kde.KLockingServer.xml klockingserver.h KLockingServer)

kde4_add_plugin(kded_klockingserver ${kded_klockingserver_SRCS})

target_link_libraries(kded_klockingserver ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${X11_LIBRARIES})

install(TARGETS kded_klockingserver DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES klockingserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
