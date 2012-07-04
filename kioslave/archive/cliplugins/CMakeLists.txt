########### next target ###############

set(cliplugins_SRCS
    cli7zplugin/cliplugin.cpp
   )

include_directories("..")

kde4_add_library(cliplugins STATIC ${cliplugins_SRCS})

target_link_libraries(cliplugins kerfuffle)
