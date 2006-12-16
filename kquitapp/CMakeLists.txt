set(kquitapp_SRCS dbusquit.cpp)

kde4_add_executable(kquitapp_executable ${dbusquit_SRCS})
set_target_properties(kquitapp_executable PROPERTIES OUTPUT_NAME kquitapp)
target_link_libraries(kquitapp_executable  ${KDE4_KDECORE_LIBS})

install(TARGETS kquitapp_executable DESTINATION ${BIN_INSTALL_DIR})
