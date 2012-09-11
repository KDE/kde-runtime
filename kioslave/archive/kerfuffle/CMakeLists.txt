macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "qjson" "A library for processing and serializing JSON files" "http://qjson.sourceforge.net" FALSE "" "Required for compiling Ark's unit tests")

########### next target ###############

set(kerfuffle2_SRCS
    archive.cpp
    archiveinterface.cpp
    jobs.cpp
    archivebase.cpp
    #extractiondialog.cpp
    #adddialog.cpp
    queries.cpp
    #addtoarchive.cpp
    cliinterface.cpp
   )

include_directories("..")

#kde4_add_kcfg_files(kerfuffle2_SRCS settings.kcfgc)

#kde4_add_ui_files(kerfuffle2_SRCS extractiondialog.ui adddialog.ui )

kde4_add_library(kerfuffle2 STATIC ${kerfuffle2_SRCS})

target_link_libraries(kerfuffle2 ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS})
if (NOT WIN32)
  target_link_libraries(kerfuffle2 ${KDE4_KPTY_LIBS})
endif (NOT WIN32)

set_target_properties(kerfuffle2 PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
set_target_properties(kerfuffle2 PROPERTIES COMPILE_FLAGS -fPIC)

#install(TARGETS kerfuffle2 ${INSTALL_TARGETS_DEFAULT_ARGS})

#install(FILES kerfuffle2Plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
#install(FILES ark.kcfg                DESTINATION ${KCFG_INSTALL_DIR})

if (QJSON_FOUND)
  # This is a hack to make QJSON work with both 0.7.1 (the latest stable)
  # and the current master (b440550), which uses a different casing for
  # the CMake variables.
  # It should be removed when QJSON master becomes sane and reverts the
  # casing again.
  if (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)
    set(KERFUFFLE_QJSON_LIBRARIES "${QJSON_LIBRARIES}")
    set(KERFUFFLE_QJSON_INCLUDE_DIR "${QJSON_INCLUDE_DIR}")
  else (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)
    set(KERFUFFLE_QJSON_LIBRARIES "${qjson_LIBRARIES}")
    set(KERFUFFLE_QJSON_INCLUDE_DIR "${qjson_INCLUDE_DIR}")
  endif (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)

  #  add_subdirectory(tests)
endif (QJSON_FOUND)
