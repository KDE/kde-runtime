set(dbusquit_SRCS dbusquit.cpp)

kde4_add_executable(dbusquit_executable NOGUI ${dbusquit_SRCS})
set_target_properties(dbusquit_executable PROPERTIES OUTPUT_NAME dbusquit)
target_link_libraries(dbusquit_executable  ${KDE4_KDECORE_LIBS})

install(TARGETS dbusquit_executable DESTINATION ${BIN_INSTALL_DIR})
