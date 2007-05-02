
project(printmgr)

add_definitions(-D_KDEPRINT_COMPILE)



########### next target ###############

set(kcm_printmgr_PART_SRCS kcmprintmgr.cpp )

kde4_automoc(${kcm_printmgr_PART_SRCS})

kde4_add_plugin(kcm_printmgr ${kcm_printmgr_PART_SRCS})


target_link_libraries(kcm_printmgr  kdeprint_management ${QT_QTGUI_LIBRARY} ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_printmgr  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES printers.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


kde4_install_icons( ${ICON_INSTALL_DIR}  )
install( FILES printmgr.desktop   DESTINATION ${DATA_INSTALL_DIR}/Settingsmenu/ )
