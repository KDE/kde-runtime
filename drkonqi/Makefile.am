# $Id$

AM_CPPFLAGS = -DQT_NO_CAST_ASCII -DQT_NO_ASCII_CAST -DQT_NO_COMPAT

SUBDIRS = presets

INCLUDES = $(all_includes)

bin_PROGRAMS = drkonqi

# Libraries:
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD  = $(LIB_KDEUI)

drkonqi_SOURCES = main.cpp toplevel.cpp

drkonqi_METASOURCES = AUTO

messages:
	$(XGETTEXT) $(drkonqi_SOURCES) -o $(podir)/drkonqi.pot
