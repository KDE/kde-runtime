#SUBDIRS= kcmaudio

INCLUDES    = $(all_includes)
METASOURCES = AUTO


# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = librenaudioplugin.la

# This is all standard.  Remove the LIB_KHTML reference if you are not
# using the KHTML Part
librenaudioplugin_la_SOURCES = audiopreview.cpp audio_plugin.cpp
librenaudioplugin_la_LIBADD = $(LIB_QT) $(LIB_KDECORE) $(LIB_KIO)
librenaudioplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir   = $(kde_servicesdir)
plugins_DATA = renaudiodlg.desktop

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)

messages: rc.cpp
	$(XGETTEXT) *.cpp  -o $(podir)/audiorename_plugin.pot
