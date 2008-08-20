set(kded_phonon_SRCS
   phononserver.cpp
   audiodevice.cpp
   audiodeviceaccess.cpp
   hardwaredatabase.cpp
   )

#kde4_add_kcfg_files(kded_phonon_SRCS ../libmediacommon/mediamanagersettings.kcfgc )

#qt4_add_dbus_adaptor(kded_phonon_SRCS org.kde.Phonon.xml serviceregistry.h)
kde4_add_plugin(kded_phononserver ${kded_phonon_SRCS})
target_link_libraries(kded_phononserver ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS})
install(TARGETS kded_phononserver  DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES phononserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

install(FILES hardwaredatabase DESTINATION ${DATA_INSTALL_DIR}/libphonon)
