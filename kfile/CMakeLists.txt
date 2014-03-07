########### kfile5 ###############
# Named this way to fix conflict with kdelibs3.
# Distros can add a kfile symlink if kdelibs3 isn't installed.

add_executable(kfile5 fileprops.cpp)
ecm_mark_nongui_executable(kfile5)

target_link_libraries(kfile5 KF5::KDE4Support)

install(TARGETS kfile5 ${INSTALL_TARGETS_DEFAULT_ARGS} )
