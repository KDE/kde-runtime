add_library(renimageplugin imagevisualizer.cpp image_plugin.cpp)
target_link_libraries(renimageplugin KF5::KDE4Support KF5::KIOCore)

install(TARGETS renimageplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES renimagedlg.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

