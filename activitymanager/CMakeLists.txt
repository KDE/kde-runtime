
set( kded_activitymanager_SRCS
    activitymanager.cpp
)

qt4_add_dbus_adaptor(
    kded_activitymanager_SRCS
    org.kde.ActivityManager.xml
    activitymanager.h
    ActivityManager
)

qt4_add_dbus_interface(
    kded_activitymanager_SRCS
    ${KDEBASE_RUNTIME_SOURCE_DIR}/../workspace/libs/kworkspace/org.kde.ActivityController.xml
    activitycontroller_interface
)

qt4_add_dbus_interface(
    kded_activitymanager_SRCS
    ${KDEBASE_RUNTIME_SOURCE_DIR}/nepomuk/services/activities/org.kde.nepomuk.ActivitiesService.xml
    nepomukactivitiesservice_interface
)

kde4_add_plugin(
    kded_activitymanager
    ${kded_activitymanager_SRCS}
)

target_link_libraries(kded_activitymanager
    ${KDE4_KDECORE_LIBS}
)

install( TARGETS kded_activitymanager DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES activitymanager.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.ActivityManager.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
