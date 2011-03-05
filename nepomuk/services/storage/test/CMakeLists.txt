set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde4_add_unit_test(crappyinferencer2test
  crappyinferencer2test.cpp
  ../crappyinferencer2.cpp
  ../typevisibilitytree.cpp)
target_link_libraries(crappyinferencer2test
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS})

kde4_add_unit_test(typevisibilitytreetest
  typevisibilitytreetest.cpp
  ../typevisibilitytree.cpp)
target_link_libraries(typevisibilitytreetest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS})
