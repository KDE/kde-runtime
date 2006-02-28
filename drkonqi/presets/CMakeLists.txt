kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/drkonqi/presets FILES enduserrc developerrc )

kde4_footer()



#original Makefile.am contents follow:

#presets_DATA = enduserrc developerrc
#presetsdir = $(kde_datadir)/drkonqi/presets
