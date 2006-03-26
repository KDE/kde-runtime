kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/drkonqi/presets FILES enduserrc developerrc )

kde4_footer()



#original Makefile.am contents follow:

#presets_DATA = enduserrc developerrc
#presetsdir = $(kde_datadir)/drkonqi/presets
