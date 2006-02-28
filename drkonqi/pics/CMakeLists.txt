kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/drkonqi/pics FILES konqi.png )

kde4_footer()



#original Makefile.am contents follow:

#drkonqi_pics_data_DATA = konqi.png
#drkonqi_pics_datadir = $(kde_datadir)/drkonqi/pics
