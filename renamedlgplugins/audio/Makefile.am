INCLUDES    = $(all_includes)
METASOURCES = AUTO


# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = librenaudioplugin.la

librenaudioplugin_la_SOURCES = audiopreview.cpp audio_plugin.cpp
librenaudioplugin_la_LIBADD = $(LIB_KIO) -lkmediaplayer
librenaudioplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir   = $(kde_servicesdir)
plugins_DATA = renaudiodlg.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp  -o $(podir)/audiorename_plugin.pot
