################# KDE PLUGIN #################

find_package(KDE4)

if (NOT KDE4_FOUND)
    message("** No KDE4 found, building attica without KDE4 plugin.")
endif (NOT KDE4_FOUND)
if (KDE4_FOUND)
    message("** KDE4 found, building attica with KDE4 plugin.")

    include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} ${ATTICA_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
    link_directories(${ATTICA_LIBRARY_DIRS})


    include(KDE4Defaults)
    include(MacroLibrary)
    include(MacroOptionalAddSubdirectory)

    set(attica_kde_SRCS
        kdeplatformdependent.cpp
    )

    kde4_add_plugin(attica_kde ${attica_kde_SRCS})
    target_link_libraries(attica_kde ${KDE4_KIO_LIBS} ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} ${ATTICA_LIBRARIES} )
    install(TARGETS attica_kde  EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (KDE4_FOUND)

