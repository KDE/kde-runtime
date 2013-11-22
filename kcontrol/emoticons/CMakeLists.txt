set(kcmemoticons_SRCS emoticonslist.cpp)
qt5_wrap_ui(kcmemoticons_SRCS emoticonslist.ui)

kde4_add_plugin(kcm_emoticons ${kcmemoticons_SRCS})
target_link_libraries(kcm_emoticons
                      KF5::KNewStuff
                      KF5::KDE4Support
                      KF5::KEmoticons)

install(TARGETS kcm_emoticons  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES emoticons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES emoticons.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
