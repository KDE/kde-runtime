set(kdontchangethehostname_SRCS khostname.cpp )
kde4_add_executable(kdontchangethehostname NOGUI ${kdontchangethehostname_SRCS})
target_link_libraries(kdontchangethehostname  ${KDE4_KDECORE_LIBS} )
install(TARGETS kdontchangethehostname DESTINATION ${LIBEXEC_INSTALL_DIR} )

