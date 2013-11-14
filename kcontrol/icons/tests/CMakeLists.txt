set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

set(testicons_SRCS testicons.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../icons.cpp )

kde4_add_executable(testicons TEST ${testicons_SRCS})

target_link_libraries(testicons
    Qt5::Widgets
    KF5::KCMUtils
    KF5::KI18n
    KF5::KDE4Support
    ${X11_LIBRARIES}
)
