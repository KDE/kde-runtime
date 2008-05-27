kde4_add_executable(kiconfinder NOGUI kiconfinder.cpp)

target_link_libraries(kiconfinder ${KDE4_KDEUI_LIBS})

install(TARGETS kiconfinder ${INSTALL_TARGETS_DEFAULT_ARGS} )
