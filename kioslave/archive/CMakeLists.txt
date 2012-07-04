add_subdirectory(kerfuffle)
add_subdirectory(cliplugins)
add_subdirectory(tests)

########### kio_archive ###############

include_directories("cliplugins")

set(kio_archive_SRCS kio_archive.cpp k7z.cpp)

kde4_add_plugin(kio_archive ${kio_archive_SRCS})

target_link_libraries(kio_archive ${KDE4_KIO_LIBS} cliplugins)

install(TARGETS kio_archive DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES tar.protocol ar.protocol zip.protocol p7zip.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )


