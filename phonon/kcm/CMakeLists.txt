macro_optional_find_package(PulseAudio 0.9.16)
set_package_properties(PulseAudio PROPERTIES DESCRIPTION "PulseAudio Audio Server"
                       URL "http://www.pulseaudio.org/"
                       TYPE OPTIONAL
                       PURPOSE "libpulse is needed for audio setup GUI"
                      )

find_package(GLIB2)
set_package_properties(GLIB2 PROPERTIES DESCRIPTION "Low-level core library for data structure handling, portability wrappers, etc."
                       URL "http://www.gtk.org"
                       TYPE OPTIONAL
                      )

pkg_check_modules(CANBERRA libcanberra)
add_feature_info ("Audio setup GUI" CANBERRA_FOUND
                  "libcanberra is needed for audio setup GUI
                  * http://0pointer.de/lennart/projects/libcanberra"
                 )

set(kcmphonon_SRCS main.cpp devicepreference.cpp backendselection.cpp)
kde4_add_ui_files(kcmphonon_SRCS devicepreference.ui backendselection.ui)
set(kcmphonon_LIBS ${KDE4_PHONON_LIBS} ${KDE4_KCMUTILS_LIBS} ${KDE4_KIO_LIBS})

if(GLIB2_FOUND AND PULSEAUDIO_FOUND AND CANBERRA_FOUND)
  add_definitions(-DHAVE_PULSEAUDIO)

  set(kcmphonon_SRCS ${kcmphonon_SRCS} audiosetup.cpp testspeakerwidget.cpp)
  kde4_add_ui_files(kcmphonon_SRCS audiosetup.ui)

  include_directories(${GLIB2_INCLUDE_DIR} ${PULSEAUDIO_INCLUDE_DIR} ${CANBERRA_INCLUDE_DIRS})

  set(kcmphonon_LIBS ${kcmphonon_LIBS} ${GLIB2_LIBRARIES} ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_MAINLOOP_LIBRARY} ${CANBERRA_LIBRARIES})
endif(GLIB2_FOUND AND PULSEAUDIO_FOUND AND CANBERRA_FOUND)

kde4_add_plugin(kcm_phonon ${kcmphonon_SRCS})
target_link_libraries(kcm_phonon ${kcmphonon_LIBS})

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
