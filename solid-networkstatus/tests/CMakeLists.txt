include_directories(${CMAKE_CURRENT_BINARY_DIR})

qt5_wrap_ui(networkingservice_SRCS networkingserviceview.ui)
qt5_add_dbus_interface(networkingservice_SRCS ../org.kde.Solid.Networking.Service.xml serviceinterface)

add_executable(networkingservice networkingservice.cpp ${networkingservice_SRCS})

target_link_libraries(networkingservice KF5::KDE4Support)

