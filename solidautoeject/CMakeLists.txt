########### next target ###############

set(kded_solidautoeject_SRCS
   solidautoeject.cpp
)

kde4_add_plugin(kded_solidautoeject ${kded_solidautoeject_SRCS})

target_link_libraries(kded_solidautoeject ${KDE4_SOLID_LIBS} ${KDE4_KDECORE_LIBS})

install(TARGETS kded_solidautoeject DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES solidautoeject.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

