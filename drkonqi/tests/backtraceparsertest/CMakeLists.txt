kde4_add_unit_test( backtraceparsertest fakebacktracegenerator.cpp backtraceparsertest.cpp)
target_link_libraries( backtraceparsertest ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} drkonqi_backtrace_parser)

kde4_add_executable(backtraceparsertest_manual TEST fakebacktracegenerator.cpp backtraceparsertest_manual.cpp)
target_link_libraries(backtraceparsertest_manual drkonqi_backtrace_parser)

# create a link so that the data directory can be accessed from the build directory, where the test runs
if( NOT WIN32 )
	if ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
		add_custom_command(TARGET backtraceparsertest POST_BUILD
						   COMMAND ${CMAKE_COMMAND} -E create_symlink
								${CMAKE_CURRENT_SOURCE_DIR}/backtraceparsertest_data backtraceparsertest_data
						   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
						  )
	endif ( NOT ${CMAKE_CURRENT_BINARY_DIR} EQUAL ${CMAKE_CURRENT_SOURCE_DIR} )
endif( NOT WIN32 )
