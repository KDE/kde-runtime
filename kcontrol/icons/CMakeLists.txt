
add_subdirectory(tests)

########### next target ###############

set(kcm_icons_PART_SRCS iconthemes.cpp icons.cpp main.cpp )


kde4_add_plugin(kcm_icons ${kcm_icons_PART_SRCS})


target_link_libraries(kcm_icons ${KDE4_KIO_LIBS} ${KDE4_KNEWSTUFF2_LIBS})

install(TARGETS kcm_icons  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES icons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES icons.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
