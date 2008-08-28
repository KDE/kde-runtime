include_directories( ${KDE4_KIO_INCLUDES} )

set(khotnewstuff_SRCS khotnewstuff.cpp )

kde4_add_executable(khotnewstuff4 ${khotnewstuff_SRCS})

target_link_libraries(khotnewstuff4 ${KDE4_KNEWSTUFF2_LIBS} ${KDE4_KDECORE_LIBS} 
                                    ${KDE4_KDEUI_LIBS} ${QT4_QTCORE_LIBRARY})

install(TARGETS khotnewstuff4 ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES khotnewstuff.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

