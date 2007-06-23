

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
########### next target ###############
set(testicons_SRCS testicons.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../icons.cpp )

kde4_automoc(${testicons_SRCS})

kde4_add_test_executable(testicons ${testicons_SRCS})

target_link_libraries(testicons ${KDE4_KDEUI_LIBS})


