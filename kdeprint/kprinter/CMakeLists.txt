



########### next target ###############

set(kprinter_KDEINIT_SRCS main.cpp printwrapper.cpp )


kde4_add_kdeinit_executable( kprinter ${kprinter_KDEINIT_SRCS})

target_link_libraries(kdeinit_kprinter ${KDE4_KIO_LIBS} ${KDE4_KDEPRINT_LIBS})

install(TARGETS kdeinit_kprinter  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kprinter kdeinit_kprinter )
install(TARGETS kprinter DESTINATION ${BIN_INSTALL_DIR})

