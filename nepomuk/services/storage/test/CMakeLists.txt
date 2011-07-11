set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde4_add_unit_test(crappyinferencer2test
  crappyinferencer2test.cpp
  ../crappyinferencer2.cpp
  ../classandpropertytree.cpp)
target_link_libraries(crappyinferencer2test
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  nepomukdatamanagement)

kde4_add_unit_test(removablemediamodeltest
  removablemediamodeltest.cpp
  ../removablemediamodel.cpp
)
add_definitions(-DFAKE_COMPUTER_XML="\\"${CMAKE_CURRENT_SOURCE_DIR}/solid/fakecomputer.xml\\"")
target_link_libraries(removablemediamodeltest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomukcommon
)

set( DMTEST
  ../classandpropertytree.cpp
  ../datamanagementmodel.cpp
  ../resourcewatchermanager.cpp
  ../resourcewatcherconnection.cpp
  ../datamanagementadaptor.cpp
  ../datamanagementcommand.cpp
  ../resourcemerger.cpp
  ../resourceidentifier.cpp
)

kde4_add_library( datamanagementtestlib STATIC ${DMTEST} )

target_link_libraries( datamanagementtestlib
  ${SOPRANO_LIBRARIES}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomukdatamanagement
  nepomuksync
)

kde4_add_unit_test(classandpropertytreetest
  classandpropertytreetest.cpp
)
target_link_libraries(classandpropertytreetest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  nepomukdatamanagement
  datamanagementtestlib
)

kde4_add_unit_test(datamanagementmodeltest
  datamanagementmodeltest.cpp
)

target_link_libraries(datamanagementmodeltest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomuksync
  nepomukdatamanagement
  datamanagementtestlib
)


kde4_add_unit_test(datamanagementadaptortest
  datamanagementadaptortest.cpp
)

target_link_libraries(datamanagementadaptortest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomuksync
  nepomukdatamanagement
  datamanagementtestlib
)


configure_file(nepomuk_dms_test_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/nepomuk_dms_test_config.h)

kde4_add_executable(fakedms
  fakedatamanagementservice.cpp
)

target_link_libraries(fakedms
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomuksync
  nepomukdatamanagement
  datamanagementtestlib
)


kde4_add_unit_test(asyncclientapitest
  asyncclientapitest.cpp
)

target_link_libraries(asyncclientapitest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_CLIENT_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomukdatamanagement
)

kde4_add_executable(resourcewatchertest
  resourcewatchertest.cpp
)

target_link_libraries(resourcewatchertest
  ${QT_QTTEST_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_LIBRARIES}
  nepomuksync
  nepomukdatamanagement
  datamanagementtestlib
)
