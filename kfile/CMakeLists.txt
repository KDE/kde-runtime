########### kfile4 ###############
# Named this way to fix conflict with kdelibs3.
# Distros can add a kfile symlink if kdelibs3 isn't installed.

set(kfile_SRCS fileprops.cpp )

kde4_add_executable(kfile4 NOGUI ${kfile_SRCS})

target_link_libraries(kfile4 ${KDE4_KIO_LIBS} )

install(TARGETS kfile4 ${INSTALL_TARGETS_DEFAULT_ARGS} )
