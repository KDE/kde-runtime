################# KDE PLUGIN for LibAttica #################

set(attica_kde_SRCS
    kdeplatformdependent.cpp
)

kde4_add_plugin(attica_kde ${attica_kde_SRCS})
target_link_libraries(attica_kde ${KDE4_KIO_LIBS} ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KCMUTILS_LIBS} )
install(TARGETS attica_kde EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

