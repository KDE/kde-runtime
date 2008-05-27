
kde4_add_executable(kde4-menu NOGUI kde-menu.cpp)

target_link_libraries(kde4-menu ${KDE4_KIO_LIBS})

install(TARGETS kde4-menu ${INSTALL_TARGETS_DEFAULT_ARGS} )

