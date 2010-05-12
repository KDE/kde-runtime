set(kcmemoticons_SRCS emoticonslist.cpp)
kde4_add_ui_files(kcmemoticons_SRCS emoticonslist.ui)

kde4_add_plugin(kcm_emoticons ${kcmemoticons_SRCS})
target_link_libraries(kcm_emoticons ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KEMOTICONS_LIBRARY})

install(TARGETS kcm_emoticons  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES emoticons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES emoticons.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
