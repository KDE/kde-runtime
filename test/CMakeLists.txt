set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde4_add_unit_test(classandpropertytreetest
  classandpropertytreetest.cpp
  ../classandpropertytree.cpp)
target_link_libraries(classandpropertytreetest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS})


kde4_add_unit_test(datamanagementmodeltest
  datamanagementmodeltest.cpp
  ../classandpropertytree.cpp
  ../datamanagementmodel.cpp
  ../resourcemerger.cpp)

target_link_libraries(datamanagementmodeltest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  nepomuksync
  nepomukdatamanagement
)

kde4_add_unit_test(datamanagementadaptortest
  datamanagementadaptortest.cpp
  ../classandpropertytree.cpp
  ../datamanagementmodel.cpp
  ../datamanagementadaptor.cpp
  ../datamanagementcommand.cpp
  ../resourcemerger.cpp)

target_link_libraries(datamanagementadaptortest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  nepomuksync
  nepomukdatamanagement
)
