add_subdirectory(tests)

set (trashcommon_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/trashimpl.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/discspaceutil.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/trashsizecache.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/kinterprocesslock.cpp
    )

########### next target ###############

if(WIN32)
  set(kio_trash_PART_SRCS kio_trash_win.cpp kio_trash_win.h)
else(WIN32)
  set(kio_trash_PART_SRCS kio_trash.cpp ${trashcommon_PART_SRCS})
endif(WIN32)

set(kio_trash_LIBS ${KDE4_KIO_LIBS} ${KDE4_SOLID_LIBS})
if(APPLE)
    set(kio_trash_LIBS ${kio_trash_LIBS} "-framework DiskArbitration")
endif(APPLE)

if(NOT MINGW)
kde4_add_plugin(kio_trash ${kio_trash_PART_SRCS})


target_link_libraries(kio_trash  ${kio_trash_LIBS})

install(TARGETS kio_trash  DESTINATION ${PLUGIN_INSTALL_DIR} )
endif(NOT MINGW)
########### next target ###############

set(ktrash_SRCS ktrash.cpp )


kde4_add_executable(ktrash NOGUI ${ktrash_SRCS})

target_link_libraries(ktrash  ${KDE4_KIO_LIBS} )

install(TARGETS ktrash ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

# currently not on win32, TODO!
if(NOT WIN32)
    set(kcm_trash_PART_SRCS kcmtrash.cpp ${trashcommon_PART_SRCS})
    kde4_add_plugin(kcm_trash ${kcm_trash_PART_SRCS})
    target_link_libraries(kcm_trash  ${KDE4_KDEUI_LIBS} ${kio_trash_LIBS})

    install(TARGETS kcm_trash DESTINATION ${PLUGIN_INSTALL_DIR})
endif(NOT WIN32)

########### install files ###############

install( FILES trash.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kcmtrash.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
