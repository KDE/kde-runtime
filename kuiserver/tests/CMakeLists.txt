
########### next target ###############

set(kuiservertest_SRCS kuiservertest.cpp)


kde4_add_test_executable(kuiservertest ${kuiservertest_SRCS})

target_link_libraries(kuiservertest ${KDE4_KIO_LIBS})

