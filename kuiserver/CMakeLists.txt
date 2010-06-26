add_subdirectory( tests )

include_directories( ${KDE4_KIO_INCLUDES} )

# for dbus_add_activation_service
include(PkgConfigGetVar)

set(kuiserver_KDEINIT_SRCS
main.cpp
uiserver.cpp
jobview.cpp
progresslistmodel.cpp
progresslistdelegate.cpp
)

set(kuiserver_KDEINIT_PRIVATE
progresslistdelegate_p.h
uiserver_p.h
)

kde4_add_ui_files(kuiserver_KDEINIT_SRCS configdialog.ui)


#<DBus Stuff>

#<JobView>
  set(jobview_xml ${KDE4_DBUS_INTERFACES_DIR}/org.kde.JobViewV2.xml)

  qt4_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${jobview_xml} jobview.h JobView jobviewadaptor )

  qt4_add_dbus_interface(kuiserver_KDEINIT_SRCS ${jobview_xml} jobview_interface )
#</JobView>


#<JobViewServer>
  set(jobviewserver_xml ${KDE4_DBUS_INTERFACES_DIR}/org.kde.JobViewServer.xml)

  qt4_add_dbus_adaptor(kuiserver_KDEINIT_SRCS ${jobviewserver_xml} progresslistmodel.h ProgressListModel jobviewserveradaptor )

  qt4_add_dbus_interface(kuiserver_KDEINIT_SRCS ${jobviewserver_xml} jobviewserver_interface )
#</JobViewServer>

qt4_add_dbus_adaptor(kuiserver_KDEINIT_SRCS org.kde.kuiserver.xml progresslistmodel.h ProgressListModel kuiserveradaptor )


dbus_add_activation_service(org.kde.kuiserver.service.in)

#</DBus Stuff>


kde4_add_kcfg_files(kuiserver_KDEINIT_SRCS kuiserversettings.kcfgc)

kde4_add_kdeinit_executable( kuiserver ${kuiserver_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuiserver  ${KDE4_KIO_LIBS} )
install(TARGETS kdeinit_kuiserver ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( kuiserver kdeinit_kuiserver )
install(TARGETS kuiserver ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES kuiserver.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
