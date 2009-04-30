if (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned_win.cpp)
else (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned.cpp)
endif (WIN32)

kde4_add_plugin(kded_ktimezoned ${kded_ktimezoned_SRCS})

#qt4_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(kded_ktimezoned ${KDE4_KIO_LIBS})

install(TARGETS kded_ktimezoned DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES ktimezoned.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.KTimeZoned.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )


