
add_subdirectory( hicolor )
add_subdirectory( emoticons )
#add_subdirectory( oxygen )

set(ksvgtopng_SRCS ksvgtopng.cpp )
kde4_add_executable(ksvgtopng NOGUI ${ksvgtopng_SRCS})
target_link_libraries(ksvgtopng ${QT_QTGUI_LIBRARY} ${QT_QTSVG_LIBRARY})
install(TARGETS ksvgtopng ${INSTALL_TARGETS_DEFAULT_ARGS} )

# create a symlink for the default icon theme,
# so that fallbacks work even for themes that don't inherit from oxygen
if (UNIX)
   # write a cmake script file which creates the symlink
   file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_icontheme_symlink.cmake "exec_program(${CMAKE_COMMAND} ARGS -E create_symlink oxygen \"\$ENV{DESTDIR}${ICON_INSTALL_DIR}/default.kde4\")\n")
   # and add it as post-install script to any of the installed targets, so it will be executed during "make install"
   install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_icontheme_symlink.cmake)
endif (UNIX)
