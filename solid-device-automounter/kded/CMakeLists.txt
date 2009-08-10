set(kded_device_automounter_SRCS DeviceAutomounter.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

kde4_add_kcfg_files(kded_device_automounter_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/AutomounterSettingsBase.kcfgc)

kde4_add_plugin(kded_device_automounter ${device_automounter_lib_SRCS} ${kded_device_automounter_SRCS})

target_link_libraries(kded_device_automounter ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS})

install(TARGETS kded_device_automounter DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES device_automounter.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
