
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


kde4_add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS drkonqi DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############





