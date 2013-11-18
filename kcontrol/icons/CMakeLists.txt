
add_subdirectory(tests)

########### next target ###############

set(kcm_icons_PART_SRCS iconthemes.cpp icons.cpp main.cpp )

kde4_add_plugin(kcm_icons ${kcm_icons_PART_SRCS})

target_link_libraries(kcm_icons
    Qt5::Widgets
    Qt5::Svg
    KF5::KCMUtils
    KF5::KI18n
    KF5::KUnitConversion
    KF5::KArchive
    ${X11_LIBRARIES}
    KF5::KDE4Support
    KF5::KNewStuff
)

install(TARGETS kcm_icons  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES icons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES icons.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
