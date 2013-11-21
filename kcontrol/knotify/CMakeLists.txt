
########### next target ###############

set(kcm_knotify_PART_SRCS knotify.cpp )


qt5_wrap_ui(kcm_knotify_PART_SRCS playersettings.ui )

kde4_add_plugin(kcm_knotify ${kcm_knotify_PART_SRCS})


target_link_libraries(kcm_knotify
                        KF5::KNotifyConfig
                        KF5::KDE4Support
                        )

install(TARGETS kcm_knotify  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


