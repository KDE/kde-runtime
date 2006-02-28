kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/drkonqi/debuggers FILES gdbrc )

kde4_footer()



#original Makefile.am contents follow:

#drkonqi_debuggers_data_DATA = gdbrc
#drkonqi_debuggers_datadir = $(kde_datadir)/drkonqi/debuggers
