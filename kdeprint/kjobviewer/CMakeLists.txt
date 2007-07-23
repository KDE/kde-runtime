

add_definitions(-D_KDEPRINT_COMPILE)



########### next target ###############

set(kjobviewer_KDEINIT_SRCS kjobviewer.cpp main.cpp )


kde4_add_kdeinit_executable( kjobviewer ${kjobviewer_KDEINIT_SRCS})

target_link_libraries(kdeinit_kjobviewer  kdeprint_management ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KDEPRINT_LIBS} )

install(TARGETS kdeinit_kjobviewer  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kjobviewer kdeinit_kjobviewer )
install(TARGETS kjobviewer DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES kjobviewer.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kjobviewerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kjobviewer )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

