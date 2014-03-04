add_executable(kdontchangethehostname khostname.cpp)
ecm_mark_nongui_executable(kdontchangethehostname)
target_link_libraries(kdontchangethehostname KF5::Service KF5::KDE4Support)
install(TARGETS kdontchangethehostname DESTINATION ${LIBEXEC_INSTALL_DIR} )

