



########### next target ###############

set(kdebugdialog_SRCS 
   main.cpp 
   kabstractdebugdialog.cpp 
   kdebugdialog.cpp 
   klistdebugdialog.cpp )

kde4_add_ui_files(kdebugdialog_SRCS kdebugdialog.ui)

kde4_add_executable(kdebugdialog ${kdebugdialog_SRCS})

target_link_libraries(kdebugdialog  ${KDE4_KDEUI_LIBS} )

install(TARGETS kdebugdialog ${INSTALL_TARGETS_DEFAULT_ARGS})

