if (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned_win.cpp)
else (WIN32)
    set(kded_ktimezoned_SRCS ktimezoned.cpp)
endif (WIN32)

kservice_desktop_to_json(ktimezoned.desktop)

add_library(kded_ktimezoned MODULE ${kded_ktimezoned_SRCS})

#qt4_add_dbus_adaptor(kded_ktimezoned_SRCS org.kde.KTimeZoned ktimezonedbus.h DBusHandler)

target_link_libraries(kded_ktimezoned
   Qt5::Core
   Qt5::DBus
   KF5::KService    # plugin factory
   KF5::KCoreAddons # KDirWatch
   KF5::KDBusAddons # kdedmodule
)

install(TARGETS kded_ktimezoned DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES ktimezoned.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded )


