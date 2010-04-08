set(kcm_attica_PART_SRCS
    providerconfigwidget.cpp
    atticamodule.cpp
)

kde4_add_ui_files(kcm_attica_PART_SRCS providermanagement.ui providerconfigwidget.ui)

kde4_add_plugin(kcm_attica ${kcm_attica_PART_SRCS})

target_link_libraries(kcm_attica ${LIBATTICA_LIBRARIES} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kcm_attica DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_attica.desktop DESTINATION ${SERVICES_INSTALL_DIR})

