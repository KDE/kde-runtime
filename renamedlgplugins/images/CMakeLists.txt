



########### next target ###############

set(renimageplugin_PART_SRCS imagevisualizer.cpp image_plugin.cpp )

kde4_add_plugin(renimageplugin WITH_PREFIX ${renimageplugin_PART_SRCS})



target_link_libraries(renimageplugin  ${KDE4_KIO_LIBS} )

install(TARGETS renimageplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES renimagedlg.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

