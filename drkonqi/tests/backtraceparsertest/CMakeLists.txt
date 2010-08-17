set( backtraceparsertest_SRCS backtraceparsertest.cpp ../../backtraceparser.cpp )

add_definitions( -DBACKTRACEPARSER_TEST )

#comment this out to see kDebug() messages from BacktraceParser
add_definitions( -DQT_NO_DEBUG_OUTPUT )

kde4_add_unit_test( backtraceparsertest ${backtraceparsertest_SRCS} )
target_link_libraries( backtraceparsertest ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDEUI_LIBRARY})

# create a link so that the data directory can be accessed from the build directory, where the test runs
if ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
    add_custom_command(TARGET backtraceparsertest POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${CMAKE_CURRENT_SOURCE_DIR}/backtraceparsertest_data backtraceparsertest_data
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      )
endif ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
