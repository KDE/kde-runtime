set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1410)

set(bugzillalibtest_SRCS
    bugzillalibtest.cpp 
    ../../bugzillalib.cpp
    ../../debugpackageinstaller.h
)

add_executable(bugzillalibtest ${bugzillalibtest_SRCS})
ecm_mark_as_test(bugzillalibtest)

target_link_libraries(bugzillalibtest ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KXMLRPCCLIENT_LIBRARY})

