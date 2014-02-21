qt5_wrap_ui_files(networkingservice_SRCS networkingserviceview.ui)
qt5_add_dbus_interface(networkingservice_SRCS ../org.kde.Solid.Networking.Service.xml serviceinterface)

add_executable(networkingservice networkingservice.cpp)

target_link_libraries(networkingservice ${KDE4_KIO_LIBS})

