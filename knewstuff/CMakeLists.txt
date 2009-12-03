include_directories( ${CMAKE_SOURCE_DIR} ${KDE4_KIO_INCLUDES} )

set(khotnewstuff_SRCS khotnewstuff.cpp )
kde4_add_executable(khotnewstuff4 ${khotnewstuff_SRCS})
target_link_libraries(khotnewstuff4 ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KDECORE_LIBS} 
                                    ${KDE4_KDEUI_LIBS} ${QT4_QTCORE_LIBRARY})
install(TARGETS khotnewstuff4 ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES khotnewstuff.knsrc DESTINATION  ${CONFIG_INSTALL_DIR} )

if(LIBATTICA_FOUND)
  include_directories(${LIBATTICA_INCLUDE_DIR})

  set(khotnewstuffupload_SRCS khotnewstuff_upload.cpp )
  kde4_add_executable(khotnewstuff-upload ${khotnewstuffupload_SRCS})
  target_link_libraries(khotnewstuff-upload ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KDECORE_LIBS} 
                                      ${KDE4_KDEUI_LIBS} ${QT4_QTCORE_LIBRARY})
  install(TARGETS khotnewstuff-upload ${INSTALL_TARGETS_DEFAULT_ARGS} )
  install( FILES khotnewstuff_upload.knsrc DESTINATION  ${CONFIG_INSTALL_DIR} )
endif(LIBATTICA_FOUND)
