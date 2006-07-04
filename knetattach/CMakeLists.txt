
project(knetattach)



########### next target ###############

set(knetattach_SRCS main.cpp )

kde4_automoc(${knetattach_SRCS})

kde4_add_ui3_files(knetattach_SRCS knetattach.ui )

kde4_add_executable(knetattach ${knetattach_SRCS})

target_link_libraries(knetattach  ${KDE4_KIO_LIBS} )

install(TARGETS knetattach DESTINATION bin)


########### install files ###############

install( FILES knetattach.desktop  DESTINATION  ${XDG_APPS_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} hicolor )




#original Makefile.am contents follow:

#INCLUDES= $(all_includes)
#
#bin_PROGRAMS = knetattach
#knetattach_SOURCES = knetattach.ui main.cpp
#knetattach_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#knetattach_LDADD   = $(LIB_KIO)
#METASOURCES = AUTO
#xdg_apps_DATA = knetattach.desktop
#KDE_ICON = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/knetattach.pot
#
#knetattach.lo: knetattach.ui knetattach.ui.h
