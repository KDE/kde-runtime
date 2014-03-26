set(kcmplatform_SRCS platform.cpp 
                     registryManager.cpp
                     menuManager.cpp
					 shellEdit.cpp
					 )
qt5_wrap_ui(kcmplatform_SRCS platform.ui 
                                   shellEdit.ui)

add_library(kcm_platform MODULE ${kcmplatform_SRCS})
target_link_libraries(kcm_platform KF5::KDE4Support 
                                   KF5::Parts 
                                   KF5::KCMUtils)

install(TARGETS kcm_platform  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES platform.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
