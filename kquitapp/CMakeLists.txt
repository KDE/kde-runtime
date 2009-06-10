set(kquitapp_SRCS kquitapp.cpp)

set(kquitapp_OUTPUT_NAME kquitapp)
kde4_add_executable(kquitapp_executable NOGUI ${kquitapp_SRCS})
set_target_properties(kquitapp_executable PROPERTIES OUTPUT_NAME kquitapp)
target_link_libraries(kquitapp_executable  ${KDE4_KDECORE_LIBS})

install(TARGETS kquitapp_executable ${INSTALL_TARGETS_DEFAULT_ARGS})
