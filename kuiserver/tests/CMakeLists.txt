set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

set(kuiservertest_SRCS kuiservertest.cpp)


kde4_add_executable(kuiservertest TEST ${kuiservertest_SRCS})

target_link_libraries(kuiservertest ${KDE4_KIO_LIBS})

