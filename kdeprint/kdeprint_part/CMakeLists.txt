

add_definitions(-D_KDEPRINT_COMPILE)



########### next target ###############

set(kdeprint_part_PART_SRCS printpart.cpp )


kde4_add_plugin(kdeprint_part WITH_PREFIX ${kdeprint_part_PART_SRCS})


target_link_libraries(kdeprint_part  kdeprint_management ${KDE4_KPARTS_LIBS}  ${QT_QTGUI_LIBRARY})

install(TARGETS kdeprint_part  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kdeprint_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kdeprint_part.rc  DESTINATION  ${DATA_INSTALL_DIR}/kdeprint_part )
