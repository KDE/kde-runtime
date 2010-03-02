project(kglobalaccel)

###############################################################################
### KDED Global Accel Daemon

include_directories( ${KDE4_KDEUI_INCLUDES} )

set(kglobalaccel_SRCS
    main.cpp
    kglobalacceld.cpp
    component.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp)

if ( Q_WS_X11 )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_x11.cpp )
endif ( Q_WS_X11 )
if ( Q_WS_MAC )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_mac.cpp )
endif ( Q_WS_MAC )
if ( Q_WS_WIN )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_win.cpp )
endif ( Q_WS_WIN )
if ( Q_WS_QWS )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_qws.cpp )
endif ( Q_WS_QWS )

kde4_add_kdeinit_executable( kglobalaccel NOGUI ${kglobalaccel_SRCS} )
target_link_libraries(kdeinit_kglobalaccel ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${X11_LIBRARIES})
if(Q_WS_MAC)
   target_link_libraries(kdeinit_kglobalaccel ${CARBON_LIBRARY})
endif(Q_WS_MAC)
target_link_libraries(kglobalaccel kdeinit_kglobalaccel)

# Install application and configuration
install( TARGETS kdeinit_kglobalaccel ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( TARGETS kglobalaccel ${INSTALL_TARGETS_DEFAULT_ARGS} )
# install( FILES kglobalaccel.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install( FILES kglobalaccel.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
# KNotify configuration
install( FILES kglobalaccel.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kglobalaccel )

# Install some update file (not yet working)
#install( FILES kconf/kdedglobalaccel_kde42.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

###
### KDE 4.2 > 4.3 Migration Start
###
### Uninstall the kde 4.0 - 4.2 kdedglobalaccel files
find_file(
    KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE
    kdedglobalaccel.desktop
    PATHS ${SERVICES_INSTALL_DIR}/kded
    NO_DEFAULT_PATH)
find_file(
    KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN
    kded_globalaccel.so
    PATHS ${PLUGIN_INSTALL_DIR}
    NO_DEFAULT_PATH)
if(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE)
    install(CODE "MESSAGE(\"Removing kdedglobalaccel desktop file\")")
    install(CODE "file(REMOVE ${KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE})")
endif(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_DESKTOP_FILE)
if(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN)
    install(CODE "MESSAGE(\"Removing kdedglobalaccel plugin\")")
    install(CODE "file(REMOVE ${KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN})")
endif(KDEBASE_KGLOBALACCEL_REMOVE_OBSOLETE_KDED_PLUGIN)

###
### KDE 4.2 > 4.3 Migration End
###

