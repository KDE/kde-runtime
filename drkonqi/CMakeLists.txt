
add_subdirectory( presets ) 
add_subdirectory( pics ) 
add_subdirectory( debuggers ) 
add_subdirectory( tests ) 

########### next target ###############

set(drkonqi_SRCS 
   main.cpp 
   debugger.cpp 
   krashconf.cpp 
   krashadaptor.cpp
   drbugreport.cpp 
   backtrace.cpp 
   toplevel.cpp )

kde4_automoc(${drkonqi_SRCS})

kde4_add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi  ${KDE4_KIO_LIBS})

install(TARGETS drkonqi DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############





#original Makefile.am contents follow:

#
#AM_CPPFLAGS = -DQT_NO_CAST_ASCII
#
#SUBDIRS = presets pics debuggers
#
#INCLUDES = $(all_includes)
#METASOURCES = AUTO
#
#EXTRA_DIST = LICENSE
#
#bin_PROGRAMS = drkonqi
#
## Libraries:
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#LDADD  = $(LIB_KDEUI) $(LIB_KIO)
#
## toplevel.cpp must be last in row due to X-headers being included. (--enable-final)
## Did I mention already that X header files really suck?
#drkonqi_SOURCES = krashdcopinterface.skel main.cpp debugger.cpp krashconf.cpp drbugreport.cpp backtrace.cpp toplevel.cpp
#
#
#check_PROGRAMS = crashtest
#crashtest_SOURCES = crashtest.cpp
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/drkonqi.pot
