INCLUDES= $(all_includes)

bin_PROGRAMS = knetattach
knetattach_SOURCES = knetattach.ui main.cpp
knetattach_LDFLAGS = $(all_libraries) $(KDE_RPATH)
knetattach_LDADD   = $(LIB_KIO)
METASOURCES = AUTO
xdg_apps_DATA = knetattach.desktop

KDE_ICON = knetattach

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/knetattach.pot

knetattach.lo: knetattach.ui knetattach.ui.h
