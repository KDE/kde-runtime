
########### next target ###############

set(kcm_knotify_PART_SRCS knotify.cpp )


kde4_add_ui_files(kcm_knotify_PART_SRCS playersettings.ui )

kde4_add_plugin(kcm_knotify ${kcm_knotify_PART_SRCS})


target_link_libraries(kcm_knotify  ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} ${QT_QTGUI_LIBRARY} )

install(TARGETS kcm_knotify  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


