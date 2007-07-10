

add_definitions(-D_KDEPRINT_COMPILE)
add_subdirectory( templates ) 



########### next target ###############

set(kio_print_PART_SRCS kio_print.cpp )


kde4_add_plugin(kio_print ${kio_print_PART_SRCS})


target_link_libraries(kio_print  ${KDE4_KIO_LIBS} ${KDE4_KDEPRINT_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTXML_LIBRARY})

install(TARGETS kio_print  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES print.protocol printdb.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

