add_subdirectory( tests )

include_directories( ${KDE4_KIO_INCLUDES} )

set(kuiserver_KDEINIT_SRCS
uiserver.cpp
progresslistmodel.cpp
progresslistdelegate.cpp
)

set(kuiserver_KDEINIT_PRIVATE
progresslistdelegate_p.h
uiserver_p.h
)

kde4_add_ui_files(kuiserver_KDEINIT_SRCS uiserverConfigDialog.ui)
qt4_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.JobView.xml uiserver.h UIServer::JobView jobviewadaptor )
qt4_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.JobViewServer.xml uiserver.h UIServer jobviewserveradaptor )
kde4_add_kcfg_files(kuiserver_KDEINIT_SRCS kuiserversettings.kcfgc)

kde4_add_kdeinit_executable( kuiserver ${kuiserver_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuiserver  ${KDE4_KIO_LIBS} )
install(TARGETS kdeinit_kuiserver ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( kuiserver kdeinit_kuiserver )
install(TARGETS kuiserver ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kuiserver.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
