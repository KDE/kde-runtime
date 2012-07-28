set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
kde4_add_unit_test(testkioarchive testkioarchive.cpp ../kcliarchive.cpp ../temporarypath.cpp ../kp7zip.cpp)
target_link_libraries(testkioarchive kerfuffle ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
