add_subdirectory( presets ) 
add_subdirectory( pics ) 
add_subdirectory( debuggers ) 
add_subdirectory( tests ) 

set(drkonqi_SRCS 
   main.cpp 
   debugger.cpp 
   krashconf.cpp 
   krashadaptor.cpp
   drbugreport.cpp 
   backtrace.cpp 
   backtracegdb.cpp
   toplevel.cpp )

kde4_add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi ${KDE4_KIO_LIBS})

install(TARGETS drkonqi DESTINATION ${LIBEXEC_INSTALL_DIR})
