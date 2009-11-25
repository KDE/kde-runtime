include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/../ocs/lib)
link_directories(${LIBATTICA_LIBRARY_DIRS})

set(kcm_attica_PART_SRCS
    providereditdialog.cpp
    atticamodule.cpp
)

kde4_add_ui_files(kcm_attica_PART_SRCS providermanagement.ui providersettingswidget.ui registerwidget.ui)

kde4_add_plugin(kcm_attica ${kcm_attica_PART_SRCS})

target_link_libraries(kcm_attica ${LIBATTICA_LIBRARIES} ${KDE4_KDEUI_LIBS})

install(TARGETS kcm_attica DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES attica.desktop DESTINATION ${SERVICES_INSTALL_DIR})

