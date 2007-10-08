
add_subdirectory( hicolor )
add_subdirectory( emoticons )
add_subdirectory( oxygen )

set(ksvgtopng_SRCS ksvgtopng.cpp )
kde4_add_executable(ksvgtopng NOGUI ${ksvgtopng_SRCS})
target_link_libraries(ksvgtopng ${KDE4_KDEUI_LIBS})
install(TARGETS ksvgtopng DESTINATION ${BIN_INSTALL_DIR} )

########### next target ###############

#set($(KSVGTOPNG)_SRCS
#dummy.cpp
#)


#kde4_add_executable($(KSVGTOPNG) ${$(KSVGTOPNG)_SRCS})

#target_link_libraries($(KSVGTOPNG)  ${KDE4_KDECORE_LIBS} )

#install(TARGETS $(KSVGTOPNG) DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

#if (UNIX)
   # write a cmake script file which creates the symlink
#   file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_icontheme_symlink.cmake "exec_program(${CMAKE_COMMAND} ARGS -E create_symlink crystalsvg ${ICON_INSTALL_DIR}/default.kde\n")
   # and add it as post-install script to any of the installed targets, so it will be executed during "make install"
#   set_target_properties(kio_uiserver PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_icontheme_symlink.cmake)
#endif (UNIX)
# TODO: INSTALL( SCRIPT .... )




