project(storage)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(storage_SRCS
  storage.cpp
  nepomukcore.cpp
  modelcopyjob.cpp
  repository.cpp
  )

set(HAVE_CLUCENE ${CLucene_FOUND})
set(HAVE_SOPRANO_INDEX ${SopranoIndex_FOUND})

if(CLucene_FOUND AND SopranoIndex_FOUND)

  include_directories(${CLUCENE_INCLUDE_DIR} ${CLUCENE_LIBRARY_DIR})

  set(storage_SRCS
    ${storage_SRCS}
    cluceneanalyzer.cpp 
    clucenetokenizer.cpp
    clucenefilter.cpp
    )

  # CLucene requires exception support and has no support for visibility=hidden
  # so we must use the default (i.e. public) value for -fvisibility
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
  IF(NOT WIN32)
    include(CheckCXXCompilerFlag)
    # Only GCC supports these flags
    IF(CMAKE_COMPILER_IS_GNUCXX)
      check_cxx_compiler_flag(-fvisibility=hidden __NEPOMUK_HAVE_GCC_VISIBILITY)
      if(__NEPOMUK_HAVE_GCC_VISIBILITY)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
      endif(__NEPOMUK_HAVE_GCC_VISIBILITY)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
  ENDIF(NOT WIN32)
endif(CLucene_FOUND AND SopranoIndex_FOUND)

configure_file(nepomukstorage-config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/nepomukstorage-config.h)

kde4_add_plugin(nepomukstorage ${storage_SRCS})

target_link_libraries(nepomukstorage
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

if(SopranoIndex_FOUND)
  target_link_libraries(nepomukstorage ${SOPRANO_INDEX_LIBRARIES})
  if(CLucene_FOUND)
    target_link_libraries(nepomukstorage ${CLUCENE_LIBRARY})
  endif(CLucene_FOUND)
endif(SopranoIndex_FOUND)

install(
  FILES nepomukstorage.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  FILES nepomukstorage.notifyrc
  DESTINATION ${DATA_INSTALL_DIR}/nepomukstorage)

install(
  TARGETS nepomukstorage
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
