project(filesystemtree_tests)

find_package(KDE4 REQUIRED)

include(KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  )

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(filesystemtreetest_SRC filesystemtreetest.cpp ../filesystemtree.cpp)

kde4_add_unit_test(filesystemtreetest TESTNAME nepomuk-filesystemtreetest NOGUI ${filesystemtreetest_SRC})

target_link_libraries(filesystemtreetest nepomuk nepomukquery
  ${QT_QTTEST_LIBRARY}
  ${KDE4_KDECORE_LIBS}
)
