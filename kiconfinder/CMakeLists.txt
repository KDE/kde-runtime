kde4_add_executable(kiconfinder NOGUI kiconfinder.cpp)

target_link_libraries(kiconfinder ${KDE4_KDEUI_LIBS})

install(TARGETS kiconfinder DESTINATION ${BIN_INSTALL_DIR} )
