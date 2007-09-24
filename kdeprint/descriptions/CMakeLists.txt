set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )




########### next target ###############
set(sortthem_SRCS sortthem.cpp )


kde4_add_executable(sortthem TEST ${sortthem_SRCS})

target_link_libraries(sortthem  ${KDE4_KDECORE_LIBS} )

########### install files ###############





