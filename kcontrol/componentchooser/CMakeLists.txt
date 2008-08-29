
add_subdirectory( componentservices ) 
add_subdirectory( windowmanagers ) 



########### next target ###############

set(kcm_componentchooser_PART_SRCS componentchooser.cpp kcm_componentchooser.cpp )

if(NOT WIN32)
  set(kcm_componentchooser_PART_SRCS ${kcm_componentchooser_PART_SRCS}
    componentchooserterminal.cpp componentchooserwm.cpp ktimerdialog.cpp)
endif(NOT WIN32)


kde4_add_ui_files(kcm_componentchooser_PART_SRCS browserconfig_ui.ui emailclientconfig_ui.ui componentchooser_ui.ui componentconfig_ui.ui) 

if(NOT WIN32)
  kde4_add_ui_files(kcm_componentchooser_PART_SRCS terminalemulatorconfig_ui.ui wmconfig_ui.ui)
endif(NOT WIN32)

kde4_add_plugin(kcm_componentchooser ${kcm_componentchooser_PART_SRCS})


target_link_libraries(kcm_componentchooser  ${KDE4_KIO_LIBS})

install(TARGETS kcm_componentchooser  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES componentchooser.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

