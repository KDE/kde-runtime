project(nepomuk_simpleresource_rcgen)
install( PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/nepomuk-simpleresource-rcgen.py 
    DESTINATION bin
    RENAME nepomuk-simpleresource-rcgen
    )

