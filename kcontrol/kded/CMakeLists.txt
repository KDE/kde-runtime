


########### next target ###############

set(kcm_kded_PART_SRCS kcmkded.cpp )


kde4_add_plugin(kcm_kded ${kcm_kded_PART_SRCS})


target_link_libraries(kcm_kded ${KDE4_KDEUI_LIBS})

install(TARGETS kcm_kded  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmkded.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
