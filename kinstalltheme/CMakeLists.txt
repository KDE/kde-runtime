


########### next target ###############

set(kinstalltheme_SRCS main.cpp )


kde4_add_executable(kinstalltheme NOGUI ${kinstalltheme_SRCS})

target_link_libraries(kinstalltheme  ${KDE4_KDECORE_LIBS} )

install(TARGETS kinstalltheme DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############





