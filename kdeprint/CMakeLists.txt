add_subdirectory( printmgr )
add_subdirectory( kdeprint_part )
if(NOT WIN32)
    add_subdirectory( kdeprintfax )
endif(NOT WIN32)
if (UNIX)
    add_subdirectory( kjobviewer )
endif(UNIX)
add_subdirectory( kprinter )

add_subdirectory( slave )
add_subdirectory( filters )

