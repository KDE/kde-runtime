include_directories( ${KDE4_KIO_INCLUDES} )

set(khotnewstuff_SRCS khotnewstuff.cpp )

kde4_add_executable(khotnewstuff4 ${khotnewstuff_SRCS})

target_link_libraries(khotnewstuff4 ${KDE4_KNEWSTUFF2_LIBS})

install(TARGETS khotnewstuff4 DESTINATION ${BIN_INSTALL_DIR} )

install( FILES khotnewstuffrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

