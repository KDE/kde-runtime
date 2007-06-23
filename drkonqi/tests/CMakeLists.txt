
set(crashtest_SRCS crashtest.cpp )

kde4_automoc(${crashtest_SRCS})

kde4_add_test_executable(crashtest ${crashtest_SRCS})

target_link_libraries(crashtest  ${KDE4_KDEUI_LIBS} )

