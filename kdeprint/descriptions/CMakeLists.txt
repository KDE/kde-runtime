



########### next target ###############
set(sortthem_SRCS sortthem.cpp )


kde4_add_test_executable(sortthem ${sortthem_SRCS})

target_link_libraries(sortthem  ${KDE4_KDECORE_LIBS} )

########### install files ###############





