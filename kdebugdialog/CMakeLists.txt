set(kdebugdialog_SRCS 
   main.cpp 
   kabstractdebugdialog.cpp 
   kdebugdialog.cpp 
   klistdebugdialog.cpp )

qt5_wrap_ui(kdebugdialog_SRCS kdebugdialog.ui)

add_executable(kdebugdialog ${kdebugdialog_SRCS})

target_link_libraries(kdebugdialog Qt5::Widgets KF5::ItemViews KF5::KDE4Support)

install(TARGETS kdebugdialog ${INSTALL_TARGETS_DEFAULT_ARGS})

