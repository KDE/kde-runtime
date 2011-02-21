project(kshutdown)

# use kded debug category for now
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7020)

set (SOURCES
    main.cpp
)
kde4_add_app_icon(SOURCES "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/actions/application-exit.png")

kde4_add_executable(kwinshutdown ${SOURCES})

target_link_libraries(kwinshutdown ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS})

########### install ###############

install( TARGETS kwinshutdown DESTINATION  ${BIN_INSTALL_DIR} )
install( PROGRAMS kwinshutdown.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
