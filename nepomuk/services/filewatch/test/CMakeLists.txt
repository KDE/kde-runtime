project(filewatch_tests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(kinotifytest_SRC kinotifytest.cpp ../kinotify.cpp)
kde4_add_unit_test(kinotifytest TESTNAME nepomuk-kinotifytest NOGUI ${kinotifytest_SRC})
target_link_libraries(kinotifytest nepomuk nepomukquery
  ${QT_QTTEST_LIBRARY}
  ${KDE4_KDECORE_LIBS}
)
