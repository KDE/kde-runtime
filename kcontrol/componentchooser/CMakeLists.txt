
add_subdirectory( componentservices ) 
add_subdirectory( windowmanagers ) 



########### next target ###############

set(kcm_componentchooser_SRCS
  componentchooser.cpp
  componentchooserbrowser.cpp
  componentchooserfilemanager.cpp
  componentchooseremail.cpp
  kcm_componentchooser.cpp
)

if(NOT WIN32)
  set(kcm_componentchooser_SRCS ${kcm_componentchooser_SRCS}
    componentchooserterminal.cpp ktimerdialog.cpp)
endif(NOT WIN32)

if(Q_WS_X11)
  set(kcm_componentchooser_SRCS ${kcm_componentchooser_SRCS}
     componentchooserwm.cpp )
endif(Q_WS_X11)


kde4_add_ui_files(kcm_componentchooser_SRCS 
  browserconfig_ui.ui
  filemanagerconfig_ui.ui
  emailclientconfig_ui.ui
  componentchooser_ui.ui
  componentconfig_ui.ui
)

if(NOT WIN32)
  kde4_add_ui_files(kcm_componentchooser_SRCS terminalemulatorconfig_ui.ui wmconfig_ui.ui)
endif(NOT WIN32)

kde4_add_plugin(kcm_componentchooser ${kcm_componentchooser_SRCS})


target_link_libraries(kcm_componentchooser  ${KDE4_KIO_LIBS})

if(WINCE)
target_link_libraries(kcm_componentchooser ${WCECOMPAT_LIBRARIES})
endif(WINCE)

install(TARGETS kcm_componentchooser  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES componentchooser.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

