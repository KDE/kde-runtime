



########### next target ###############

set(renaudioplugin_PART_SRCS audiopreview.cpp audio_plugin.cpp )

kde4_automoc(${renaudioplugin_PART_SRCS})

kde4_add_plugin(renaudioplugin WITH_PREFIX ${renaudioplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} renaudioplugin )

target_link_libraries(renaudioplugin  ${KDE4_KIO_LIBS} kmediaplayer )

install(TARGETS renaudioplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES renaudiodlg.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

