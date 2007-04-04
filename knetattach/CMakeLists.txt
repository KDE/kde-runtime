
project(knetattach)



add_definitions (-DQT3_SUPPORT)

########### next target ###############

set(knetattach_SRCS main.cpp knetattach.cpp)

kde4_automoc(${knetattach_SRCS})

kde4_add_ui_files(knetattach_SRCS knetattach.ui )

kde4_add_executable(knetattach ${knetattach_SRCS})

target_link_libraries(knetattach  ${KDE4_KIO_LIBS} )

install(TARGETS knetattach DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES knetattach.desktop  DESTINATION  ${XDG_APPS_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

