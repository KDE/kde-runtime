


########### next target ###############

set(kcm_kded_PART_SRCS kcmkded.cpp )


add_library(kcm_kded MODULE ${kcm_kded_PART_SRCS})


target_link_libraries(kcm_kded ${KDE4_KDEUI_LIBS})

install(TARGETS kcm_kded  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmkded.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
