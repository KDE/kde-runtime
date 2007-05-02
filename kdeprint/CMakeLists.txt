if(NOT WIN32)
add_subdirectory( printmgr ) 
add_subdirectory( kdeprint_part )
endif(NOT WIN32)
if (UNIX)
	add_subdirectory( kjobviewer ) 
endif(UNIX)
add_subdirectory( kprinter ) 

add_subdirectory( kdeprintfax ) 
add_subdirectory( slave ) 

