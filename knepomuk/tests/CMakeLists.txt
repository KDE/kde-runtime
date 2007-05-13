project(knepomuk_tests)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${SOPRANO_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
)

# The basis of all the tests handling statements
kde4_automoc(testbase.cpp)
kde4_add_library(testbase_backbone STATIC testbase.cpp)
target_link_libraries(testbase_backbone
  ${KNEPOMUK_LIBRARIES}
  ${KDE4_KDECORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)

# tests
kde4_automoc(rdfrepositorytest.cpp)
kde4_add_executable(rdfrepositorytest rdfrepositorytest.cpp)
target_link_libraries(rdfrepositorytest testbase_backbone)
add_test(rdfrepositorytest ${EXECUTABLE_OUTPUT_PATH}/rdfrepositorytest)

kde4_automoc(statementiteratortest.cpp)
kde4_add_executable(statementiteratortest statementiteratortest.cpp)
target_link_libraries(statementiteratortest testbase_backbone)
add_test(statementiteratortest ${EXECUTABLE_OUTPUT_PATH}/statementiteratortest)
