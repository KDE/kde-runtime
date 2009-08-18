project( contextmenu )

set(contextsrc kdecm.cpp)

if(MINGW)
    find_program(WINDRES_EXECUTABLE NAMES windres)

    set(contextsrc ${contextsrc} ${CMAKE_CURRENT_BINARY_DIR}/kdecm_res.o)

    add_custom_command(OUTPUT kdecm_res.o
        COMMAND ${WINDRES_EXECUTABLE} ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/kdecm.rc -o kdecm_res.o --include-dir=${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${WINDRES_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/kdecm.rc
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_definitions(-DUNICODE)
else(MINGW)
    set(contextsrc ${contextsrc} kdecm.rc)
endif(MINGW)
set(contextsrc ${contextsrc} kdecm.def)
add_library( kdecm SHARED ${contextsrc} )

if(MINGW)
    set_target_properties(kdecm PROPERTIES LINK_FLAGS "-Wl,--kill-at")
endif(MINGW)


target_link_libraries(kdecm ole32 comctl32 uuid ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS})

install(TARGETS kdecm ${INSTALL_TARGETS_DEFAULT_ARGS})
