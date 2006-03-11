kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(renimageplugin_PART_SRCS imagevisualizer.cpp image_plugin.cpp )

kde4_automoc(${renimageplugin_PART_SRCS})

kde4_add_plugin(renimageplugin WITH_PREFIX ${renimageplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} renimageplugin )

target_link_libraries(renimageplugin  ${QT_AND_KDECORE_LIBS} kio )

install_targets(${LIB_INSTALL_DIR}/kde4 renimageplugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES renimagedlg.desktop )

kde4_footer()



#original Makefile.am contents follow:

##SUBDIRS= kcmimage
#
#INCLUDES    = $(all_includes)
#METASOURCES = AUTO
#
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = librenimageplugin.la
#
#librenimageplugin_la_SOURCES = imagevisualizer.cpp image_plugin.cpp
#librenimageplugin_la_LIBADD = $(LIB_KIO)
#librenimageplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir   = $(kde_servicesdir)
#plugins_DATA = renimagedlg.desktop
#
## Install the .rc file in the Part's directory (in this case, the part
## is KHTMLPart)
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp  -o $(podir)/imagerename_plugin.pot
