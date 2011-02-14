project(kshutdown)

# use kded debug category for now
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7020)

set (SOURCES
    main.cpp
)
kde4_add_app_icon(SOURCES "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/actions/application-exit.png")

kde4_add_executable(kshutdown ${SOURCES})

target_link_libraries(kshutdown ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})

########### install ###############

install( TARGETS kshutdown DESTINATION  ${BIN_INSTALL_DIR} )
install( PROGRAMS kshutdown.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
