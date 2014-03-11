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

qt5_add_dbus_adaptor(kded_winstartmenu_SRCS org.kde.WinStartMenu.xml winstartmenu.h WinStartMenuModule winstartmenu_adaptor WinStartMenuAdaptor)
add_library(kded_winstartmenu MODULE ${kded_winstartmenu_SRCS})
target_link_libraries( kded_winstartmenu KF5::KIOWidgets ole32 uuid )

########### install ###############

install(TARGETS kded_winstartmenu  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES winstartmenu.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.WinStartMenu.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
install( FILES kwinstartmenurc DESTINATION  ${CONFIG_INSTALL_DIR} )

########### next target ###############

add_executable(kwinstartmenu main.cpp ${shared_SOURCES})
ecm_mark_nongui_executable(kwinstartmenu)
target_link_libraries(kwinstartmenu KF5::KIOWidgets ole32 uuid )

########### install ###############

install( TARGETS kwinstartmenu DESTINATION  ${BIN_INSTALL_DIR} )
