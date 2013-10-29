set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(crashtest_SRCS crashtest.cpp )


kde4_add_executable(crashtest TEST ${crashtest_SRCS})

target_link_libraries(crashtest ${KCore_LIBRARIES} KF5::KI18n KF5::KCrash ${KDE4_KIO_LIBS}  KF5::KDE4Support ${QT_QTCONCURRENT_LIBRARY})

