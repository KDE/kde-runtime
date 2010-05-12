set(kcmplatform_SRCS platform.cpp 
                     registryManager.cpp
                     menuManager.cpp
					 shellEdit.cpp
					 )
kde4_add_ui_files(kcmplatform_SRCS platform.ui 
                                   shellEdit.ui)

kde4_add_plugin(kcm_platform ${kcmplatform_SRCS})
target_link_libraries(kcm_platform ${KDE4_KDEUI_LIBS} 
                                   ${KDE4_KPARTS_LIBS} 
                                   ${KDE4_KCMUTILS_LIBRARY})

install(TARGETS kcm_platform  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES platform.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
