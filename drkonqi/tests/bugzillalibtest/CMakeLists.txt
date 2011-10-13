set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1410)

set(bugzillalibtest_SRCS
    bugzillalibtest.cpp 
    ../../bugzillalib.cpp
    ../../debugpackageinstaller.h
    ../../findconfigdatajob.cpp
)

kde4_add_executable(bugzillalibtest TEST ${bugzillalibtest_SRCS})

target_link_libraries(bugzillalibtest ${KDE4_KIO_LIBS})

