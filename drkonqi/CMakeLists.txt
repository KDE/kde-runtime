check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)
configure_file (config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h )

add_subdirectory( pics ) 
add_subdirectory( debuggers ) 
add_subdirectory( tests ) 

set(drkonqi_SRCS 
   main.cpp 
   krashconf.cpp 
   drkonqiadaptor.cpp
   bugzillalib.cpp
   drkonqidialog.cpp
   statuswidget.cpp
   reportinfo.cpp
   aboutbugreportingdialog.cpp
   drkonqiassistantpages_base.cpp
   drkonqiassistantpages_bugzilla.cpp
   drkonqibugreport.cpp
   usefulnessmeter.cpp
   getbacktracewidget.cpp
   backtracegenerator.cpp 
   backtraceparser.cpp
   drkonqi.cpp
   detachedprocessmonitor.cpp
   productmapping.cpp
)

kde4_add_ui_files(drkonqi_SRCS ui/getbacktracewidget.ui
    ui/assistantpage_introduction.ui ui/assistantpage_bugawareness.ui
    ui/assistantpage_conclusions.ui  ui/assistantpage_conclusions_dialog.ui
    ui/assistantpage_bugzilla_login.ui ui/assistantpage_bugzilla_keywords.ui
    ui/assistantpage_bugzilla_duplicates.ui ui/assistantpage_bugzilla_duplicates_dialog.ui
    ui/assistantpage_bugzilla_information.ui ui/assistantpage_bugzilla_send.ui)

# if BACKTRACE_PARSER_DEBUG is enabled, it will show both the
# parsed and the unparsed backtrace in the backtrace widget.
# Comment this out for release.
#add_definitions(-DBACKTRACE_PARSER_DEBUG)

kde4_add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi ${KDE4_KIO_LIBS})

install(TARGETS drkonqi DESTINATION ${LIBEXEC_INSTALL_DIR})
install(FILES mappings DESTINATION ${DATA_INSTALL_DIR}/drkonqi)