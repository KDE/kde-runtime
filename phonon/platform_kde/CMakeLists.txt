include_directories(${KDE4_KIO_INCLUDES} ${ALSA_INCLUDES})
set(kde_PART_SRCS
	kdepluginfactory.cpp
	kiomediastream.cpp
   devicelisting.cpp
	)

if(ALSA_FOUND)
   qt4_add_resources(kde_PART_SRCS resources.qrc)
endif(ALSA_FOUND)

kde4_add_plugin(kde ${kde_PART_SRCS})
target_link_libraries(kde ${KDE4_PHONON_LIBS} ${KDE4_KIO_LIBS} ${ASOUND_LIBRARY})
if(NOT WIN32)
  set_target_properties(kde PROPERTIES
                            INSTALL_RPATH_USE_LINK_PATH TRUE
                            SKIP_BUILD_RPATH TRUE
                            BUILD_WITH_INSTALL_RPATH TRUE
                            INSTALL_RPATH ${LIB_INSTALL_DIR}
                        )
endif(NOT WIN32)
install(TARGETS kde DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_platform)
install(FILES phonon.notifyrc DESTINATION ${DATA_INSTALL_DIR}/phonon)
install(FILES phononbackend.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
