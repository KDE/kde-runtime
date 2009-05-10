# install the cmake files

# don't glob for the files, we don't want to install e.g. FindLibKNotification-1.cmake
set(cmakeFiles FindCLucene.cmake
               FindSLP.cmake)

set(module_install_dir ${DATA_INSTALL_DIR}/cmake/modules )

install( FILES  ${cmakeFiles} DESTINATION ${module_install_dir} )

