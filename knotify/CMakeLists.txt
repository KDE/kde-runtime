
########### next target ###############

set(knotify_SRCS
main.cpp
knotify.cpp
knotifyplugin.cpp
knotifyconfig.cpp
notifybysound.cpp
notifybypopup.cpp
notifybylogfile.cpp
notifybytaskbar.cpp
notifybyexecute.cpp
)


kde4_add_executable( knotify ${knotify_SRCS})

target_link_libraries( knotify ${KDE4_KDEUI_LIBS} ${KDE4_PHONON_LIBS})

set_target_properties(knotify PROPERTIES OUTPUT_NAME knotify4 )
install(TARGETS knotify DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES kde.notifyrc  DESTINATION  ${DATA_INSTALL_DIR}/kde )
install( FILES knotify4.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

########### D-Bus Autostart Services #########


configure_file(org.kde.knotify.service.cmake
	  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.knotify.service)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.knotify.service DESTINATION ${DBUS_SERVICES_DIR} )




