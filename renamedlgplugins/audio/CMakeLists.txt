add_library(renaudioplugin audiopreview.cpp audio_plugin.cpp )
target_link_libraries(renaudioplugin  KF5::KIOCore kmediaplayer )

install(TARGETS renaudioplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES renaudiodlg.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

