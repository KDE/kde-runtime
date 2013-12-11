
add_subdirectory( pics )

########### next target ###############

set(kcm_locale_PART_SRCS kcmlocale.cpp)

kde4_add_ui_files(kcm_locale_PART_SRCS kcmlocalewidget.ui )

kde4_add_plugin(kcm_locale ${kcm_locale_PART_SRCS})

target_link_libraries(kcm_locale
    Qt5::Widgets
    KF5::KCMUtils
    KF5::I18n
    KF5::UnitConversion
    ${X11_LIBRARIES}
    KF5::KDE4Support
)


install(TARGETS kcm_locale  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES language.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

