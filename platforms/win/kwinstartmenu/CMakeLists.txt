project(winstartmenu)

# use kded debug category for now
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7020)

set (shared_SOURCES
    linkfile.cpp
    linkfile.h
    misc.cpp
    misc.h
)

set(kded_winstartmenu_SRCS 
    winstartmenu.cpp 
    ${shared_SOURCES}
)

qt4_add_dbus_adaptor(kded_winstartmenu_SRCS org.kde.WinStartMenu.xml winstartmenu.h WinStartMenuModule winstartmenu_adaptor WinStartMenuAdaptor)
kde4_add_plugin(kded_winstartmenu ${kded_winstartmenu_SRCS})
target_link_libraries( kded_winstartmenu ${KDE4_KIO_LIBS} ole32 uuid )

########### install ###############

install(TARGETS kded_winstartmenu  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES winstartmenu.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.WinStartMenu.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES kwinstartmenurc DESTINATION  ${CONFIG_INSTALL_DIR} )

########### next target ###############

set(winstartmenu_SRCS 
    main.cpp 
    ${shared_SOURCES}
)

kde4_add_executable(kwinstartmenu NOGUI ${winstartmenu_SRCS})
target_link_libraries(kwinstartmenu ${KDE4_KIO_LIBS} ole32 uuid )

########### install ###############

install( TARGETS kwinstartmenu DESTINATION  ${BIN_INSTALL_DIR} )
