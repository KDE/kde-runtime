########### kfile4 ###############
# Named this way to fix conflict with kdelibs3.
# Distros can add a kfile symlink if kdelibs3 isn't installed.

add_executable(kfile4 fileprops.cpp)
ecm_mark_nongui_executable(kfile4)

target_link_libraries(kfile4 ${KDE4_KIO_LIBS} )

install(TARGETS kfile4 ${INSTALL_TARGETS_DEFAULT_ARGS} )
