set( backtraceinfotest_SRCS backtraceinfotest.cpp ../../backtraceparser.cpp )

add_definitions( -DBACKTRACEPARSER_TEST )
add_definitions( -DQT_NO_DEBUG_OUTPUT )

kde4_add_unit_test( backtraceinfotest ${backtraceinfotest_SRCS} )
target_link_libraries( backtraceinfotest ${QT_QTTEST_LIBRARY} )

# create a link so that the data directory can be accessed from the build directory, where the test runs
if ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
    add_custom_command(TARGET backtraceinfotest POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${CMAKE_CURRENT_SOURCE_DIR}/backtraceinfotest_data backtraceinfotest_data
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      )
endif ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
