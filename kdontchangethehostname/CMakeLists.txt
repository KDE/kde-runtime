add_executable(kdontchangethehostname khostname.cpp)
ecm_mark_nongui_executable(kdontchangethehostname)
target_link_libraries(kdontchangethehostname  ${KDE4_KDECORE_LIBS} )
install(TARGETS kdontchangethehostname DESTINATION ${LIBEXEC_INSTALL_DIR} )

