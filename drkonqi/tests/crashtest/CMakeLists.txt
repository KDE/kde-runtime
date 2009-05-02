set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(crashtest_SRCS crashtest.cpp )


kde4_add_executable(crashtest TEST ${crashtest_SRCS})

target_link_libraries(crashtest  ${KDE4_KDEUI_LIBS} )

