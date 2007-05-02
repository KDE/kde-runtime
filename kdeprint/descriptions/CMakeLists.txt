
project(kdeprint-descriptions)



########### next target ###############
if(KDE4_BUILD_TESTS)
set(sortthem_SRCS sortthem.cpp )

kde4_automoc(${sortthem_SRCS})

kde4_add_executable(sortthem ${sortthem_SRCS})

target_link_libraries(sortthem  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############





#original Makefile.am contents follow:

#KDE_OPTIONS = qtonly
## Note: sortthem does not sort! (It is just a sort of uniq(1)
#
#INCLUDES = $(all_includes)
#
#EXTRA_PROGRAMS = sortthem
#
#sortthem_SOURCES = sortthem.cpp
#sortthem_LDFLAGS = $(all_libraries) $(LIB_QT)
#
#printers.cpp: sortthem
#	: > printers.cpp
#	for i in `find /usr/share/cups/model -type f ! -name '*.gz'`; do perl $(srcdir)/readppd < $$i >> printers.cpp; done
#	for i in `find /usr/share/cups/model -type f -name '*.gz'`; do \
#		gzip -cd $$i | perl $(srcdir)/readppd >> printers.cpp; done
#	iconv -flatin1 -tutf-8 printers.cpp | grep  -v '"[-0-9][0-9]*")' > printers.new
#	mv printers.new printers.cpp
#	./sortthem < printers.cpp | perl -n -e "print \"// xgettext: no-c-format\n\".\$$_;" > printers.new && mv printers.new $(srcdir)/printers.cpp
#
#messages:
#	$(XGETTEXT) printers.cpp -o $(podir)/../kdelibs/ppdtranslations.pot
#
#.PHONY: printers.cpp
#
