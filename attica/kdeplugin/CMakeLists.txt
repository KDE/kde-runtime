################# KDE PLUGIN for LibAttica #################

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} ${LIBATTICA_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )

set(attica_kde_SRCS
    kdeplatformdependent.cpp
)

kde4_add_plugin(attica_kde ${attica_kde_SRCS})
target_link_libraries(attica_kde ${KDE4_KIO_LIBS} ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} )
install(TARGETS attica_kde EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

