set(kcm_device_automounter_SRCS DeviceAutomounterKCM.cpp DeviceModel.cpp)

qt5_wrap_ui(kcm_device_automounter_SRCS DeviceAutomounterKCM.ui)

kconfig_add_kcfg_files(kcm_device_automounter_SRCS ../lib/AutomounterSettingsBase.kcfgc LayoutSettings.kcfgc)

add_library(kcm_device_automounter MODULE ${kcm_device_automounter_SRCS} ${device_automounter_lib_SRCS})

target_link_libraries(kcm_device_automounter KF5::DBusAddons KF5::KCMUtils KF5::Solid KF5::KDE4Support)

install(TARGETS kcm_device_automounter DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES device_automounter_kcm.desktop DESTINATION ${SERVICES_INSTALL_DIR})
