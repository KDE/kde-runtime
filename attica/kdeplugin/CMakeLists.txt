################# KDE PLUGIN for LibAttica #################

set(attica_kde_SRCS
    kdeplatformdependent.cpp
)

add_library(attica_kde MODULE ${attica_kde_SRCS})
target_link_libraries(attica_kde KF5::KIOWidgets ${QT_QTMAIN_LIBRARY} Qt5::Core KF5::KCMUtils )
install(TARGETS attica_kde EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

