set(dialog_helper_SRCS
   main.cpp
   dialoghelper.cpp
)

kde4_add_executable(dialog_helper ${dialog_helper_SRCS})

target_link_libraries(dialog_helper ${KDE4_KDEUI_LIBS})

install(TARGETS dialog_helper DESTINATION ${LIBEXEC_INSTALL_DIR})
