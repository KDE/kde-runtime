


########### next target ###############

set(kcm_icons_PART_SRCS iconthemes.cpp icons.cpp main.cpp )

kde4_automoc(${kcm_icons_PART_SRCS})

kde4_add_plugin(kcm_icons ${kcm_icons_PART_SRCS})


target_link_libraries(kcm_icons ${KDE4_KIO_LIBS})

install(TARGETS kcm_icons  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############
set(testicons_SRCS testicons.cpp icons.cpp )

kde4_automoc(${testicons_SRCS})

kde4_add_test_executable(testicons ${testicons_SRCS})

target_link_libraries(testicons ${KDE4_KDEUI_LIBS})

########### install files ###############

install( FILES icons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

