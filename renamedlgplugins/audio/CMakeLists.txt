



########### next target ###############

set(renaudioplugin_PART_SRCS audiopreview.cpp audio_plugin.cpp )

kde4_automoc(${renaudioplugin_PART_SRCS})

kde4_add_plugin(renaudioplugin WITH_PREFIX ${renaudioplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} renaudioplugin )

target_link_libraries(renaudioplugin  ${KDE4_KIO_LIBS} kmediaplayer )

install_targets(${PLUGIN_INSTALL_DIR} renaudioplugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES renaudiodlg.desktop )




#original Makefile.am contents follow:

#INCLUDES    = $(all_includes)
#METASOURCES = AUTO
#
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = librenaudioplugin.la
#
#librenaudioplugin_la_SOURCES = audiopreview.cpp audio_plugin.cpp
#librenaudioplugin_la_LIBADD = $(LIB_KIO) -lkmediaplayer
#librenaudioplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir   = $(kde_servicesdir)
#plugins_DATA = renaudiodlg.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp  -o $(podir)/audiorename_plugin.pot
