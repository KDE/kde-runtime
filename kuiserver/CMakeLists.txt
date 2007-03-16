add_subdirectory( pics )
add_subdirectory( tests )


set(kuiserver_KDEINIT_SRCS
uiserver.cpp
uiserveradaptor.cpp
progresslistmodel.cpp
progresslistdelegate.cpp
)

set(kuiserver_KDEINIT_PRIVATE
progresslistdelegate_p.h
uiserver_p.h
)

kde4_add_ui_files(kuiserver_KDEINIT_SRCS uiserverConfigDialog.ui)
kde4_automoc(${kuiserver_KDEINIT_SRCS} ${kuiserver_KDEINIT_PRIVATE})
qt4_add_dbus_interface( kuiserver_KDEINIT_SRCS ${DBUS_INTERFACES_DIR}/org.kde.KIO.Observer.xml observeriface )
kde4_add_kcfg_files(kuiserver_KDEINIT_SRCS kuiserversettings.kcfgc)

kde4_add_kdeinit_executable( kuiserver ${kuiserver_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuiserver  ${KDE4_KIO_LIBS} )
install(TARGETS kdeinit_kuiserver  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kuiserver kdeinit_kuiserver )
install(TARGETS kuiserver DESTINATION ${BIN_INSTALL_DIR} )

install( FILES kuiserver.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
