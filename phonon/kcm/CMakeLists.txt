add_subdirectory(xine)

set(kcmphonon_SRCS main.cpp devicepreference.cpp backendselection.cpp)
kde4_add_ui_files(kcmphonon_SRCS devicepreference.ui backendselection.ui)

kde4_add_plugin(kcm_phonon ${kcmphonon_SRCS})
target_link_libraries(kcm_phonon ${KDE4_PHONON_LIBS} ${KDE4_KCMUTILS_LIBS}
   ${KDE4_KIO_LIBS})

install(TARGETS kcm_phonon  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES kcm_phonon.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install(FILES listview-background.png DESTINATION ${DATA_INSTALL_DIR}/kcm_phonon)
install(FILES devicepreference.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

########### update helper ###############
kde4_add_executable(phonon_devicepreference_update devicepreference_update.cpp)
target_link_libraries(phonon_devicepreference_update ${KDE4_KDECORE_LIBS})
install(TARGETS phonon_devicepreference_update DESTINATION ${LIB_INSTALL_DIR}/kconf_update_bin)

kde4_add_executable(phonon_deviceuids_update deviceuids_update.cpp)
target_link_libraries(phonon_deviceuids_update ${KDE4_KDECORE_LIBS})
install(TARGETS phonon_deviceuids_update DESTINATION ${LIB_INSTALL_DIR}/kconf_update_bin)
