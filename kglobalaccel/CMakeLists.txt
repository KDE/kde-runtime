project(kglobalaccel)

###############################################################################
### KDED Global Accel Daemon
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=181)

include_directories( ${KDE4_KDEUI_INCLUDES} )

set(kglobalaccel_SRCS
    main.cpp
    kglobalaccel.cpp
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
target_link_libraries(kdeinit_kglobalaccel ${KDE4_KDEUI_LIBS} ${X11_LIBRARIES})
if(Q_WS_MAC)
   target_link_libraries(kdeinit_kglobalaccel ${CARBON_LIBRARY})
endif(Q_WS_MAC)
target_link_libraries(kglobalaccel kdeinit_kglobalaccel)

# Install application and configuration
install( TARGETS kdeinit_kglobalaccel ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( TARGETS kglobalaccel ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kglobalaccel.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})

# Install some update file (not yet working)
#install( FILES kconf/kdedglobalaccel_kde42.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

# Uninstall the kde 4.0 - 4.2 kdedglobalaccel files
find_file(
    KDED_DESKTOP
    kdedglobalaccel.desktop
    PATHS ${SERVICES_INSTALL_DIR}/kded
    NO_DEFAULT_PATH)
message(${KDED_DESKTOP})
find_file(
    KDED_PLUGIN
    kded_globalaccel.so
    PATHS ${PLUGIN_INSTALL_DIR}
    NO_DEFAULT_PATH)
message(${KDED_PLUGIN})

# Remove old kdedglobalaccel files
if(NOT KDED_DESKTOP STREQUAL "KDED_DESKTOP-NOTFOUND")
    install(CODE "MESSAGE(\"Removing kdedglobalaccel leftovers\")")
    install(CODE "file(REMOVE ${KDED_DESKTOP})")
endif(NOT KDED_DESKTOP STREQUAL "KDED_DESKTOP-NOTFOUND")
if(NOT KDED_PLUGIN STREQUAL "KDED_PLUGIN-NOTFOUND")
    install(CODE "MESSAGE(\"Removing kdedglobalaccel leftovers\")")
    install(CODE "file(REMOVE ${KDED_PLUGIN})")
endif(NOT KDED_PLUGIN STREQUAL "KDED_PLUGIN-NOTFOUND")

