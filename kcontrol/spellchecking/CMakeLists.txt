set(kcm_spellchecking_SRCS
   spellchecking.cpp
   )

kde4_add_plugin(kcmspellchecking ${kcm_spellchecking_SRCS})

target_link_libraries(kcmspellchecking
                      KF5::SonnetUi
                      KF5::KDE4Support
                     )

install(TARGETS kcmspellchecking   DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES spellchecking.desktop   DESTINATION  ${SERVICES_INSTALL_DIR} )
