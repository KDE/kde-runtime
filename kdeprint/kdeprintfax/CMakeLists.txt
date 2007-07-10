


add_definitions (-DQT3_SUPPORT)

########### next target ###############

set(kdeprintfax_SRCS 
   main.cpp 
   kdeprintfax.cpp 
   faxab.cpp 
   faxctrl.cpp 
   confgeneral.cpp 
   configdlg.cpp 
   conffax.cpp 
   confsystem.cpp 
   conffilters.cpp 
   filterdlg.cpp 
   defcmds.cpp )


kde4_add_executable(kdeprintfax ${kdeprintfax_SRCS})

target_link_libraries(kdeprintfax   ${KDE4_KDEPRINT_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kdeprintfax DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES kdeprintfax.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kdeprintfaxui.rc faxfilters  DESTINATION  ${DATA_INSTALL_DIR}/kdeprintfax )

install( PROGRAMS anytops  DESTINATION  ${DATA_INSTALL_DIR}/kdeprintfax )
kde4_install_icons( ${ICON_INSTALL_DIR}  )

