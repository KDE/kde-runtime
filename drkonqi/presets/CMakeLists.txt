kde4_header()



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/drkonqi/presets FILES enduserrc developerrc )

kde4_footer()



#original Makefile.am contents follow:

#presets_DATA = enduserrc developerrc
#presetsdir = $(kde_datadir)/drkonqi/presets
