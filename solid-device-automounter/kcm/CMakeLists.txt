set(kcm_device_automounter_SRCS DeviceAutomounterKCM.cpp DeviceModel.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})



kde4_add_ui_files(kcm_device_automounter_SRCS DeviceAutomounterKCM.ui)

kde4_add_kcfg_files(kcm_device_automounter_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/AutomounterSettingsBase.kcfgc ${CMAKE_CURRENT_SOURCE_DIR}/LayoutSettings.kcfgc)


kde4_add_plugin(kcm_device_automounter ${kcm_device_automounter_SRCS} ${device_automounter_lib_SRCS})

target_link_libraries(kcm_device_automounter ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS})

install(TARGETS kcm_device_automounter DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES device_automounter_kcm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
