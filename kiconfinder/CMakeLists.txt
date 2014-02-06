add_executable(kiconfinder kiconfinder.cpp)

target_link_libraries(kiconfinder KF5::IconThemes)

install(TARGETS kiconfinder ${INSTALL_TARGETS_DEFAULT_ARGS} )
