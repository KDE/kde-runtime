
project(kdebugdialog)



########### next target ###############

set(kdebugdialog_SRCS 
   main.cpp 
   kabstractdebugdialog.cpp 
   kdebugdialog.cpp 
   klistdebugdialog.cpp )

kde4_automoc(${kdebugdialog_SRCS})

kde4_add_executable(kdebugdialog ${kdebugdialog_SRCS})

target_link_libraries(kdebugdialog  ${KDE4_KDEUI_LIBS} )

install(TARGETS kdebugdialog DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############





#original Makefile.am contents follow:

### Makefile.am of kdebugdialog
#
#INCLUDES=	$(all_includes)
#
######## Files
#
#bin_PROGRAMS = kdebugdialog
#
#kdebugdialog_SOURCES = main.cpp kabstractdebugdialog.cpp kdebugdialog.cpp klistdebugdialog.cpp
#kdebugdialog_METASOURCES = AUTO
#kdebugdialog_LDFLAGS =       $(all_libraries) $(KDE_RPATH)
#kdebugdialog_LDADD   =       $(LIB_KDEUI)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kdebugdialog.pot	
#
