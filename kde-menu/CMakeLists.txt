
add_executable(kde4-menu kde-menu.cpp)
ecm_mark_nongui_executable(kde4-menu)

target_link_libraries(kde4-menu KF5::KIOWidgets KF5::KDE4Support)

install(TARGETS kde4-menu ${INSTALL_TARGETS_DEFAULT_ARGS} )

