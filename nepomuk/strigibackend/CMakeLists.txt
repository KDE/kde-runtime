project(strigi_soprano_indexer)

include_directories( 
  ${STRIGI_INCLUDE_DIR}
  ${SOPRANO_INCLUDE_DIR}
  ${CLUCENE_LIBRARY_DIR}
  ${CLUCENE_INCLUDE_DIR}
  ${QT_INCLUDES}
)

set( strigi_soprano_indexer_SRCS
  sopranoindexmanager.cpp
  sopranoindexreader.cpp
  sopranoindexwriter.cpp
  util.cpp
  tstring.cpp
)

# CLucene requires exception support
IF(NOT WIN32)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDIF(NOT WIN32)

if(WIN32)
  # this is needed to have mingw, cygwin and msvc libs installed in one directory
  if(MSVC)
    set(prefix msvc_strigiindex_)
  elseif(CYGWIN)
    set(prefix cyg_strigiindex_)
  elseif(MINGW)
    set(prefix mingw_strigiindex_)
  endif(MSVC)
else(WIN32)
  set(prefix strigiindex_)
endif(WIN32)

add_library(sopranobackend MODULE ${strigi_soprano_indexer_SRCS})

set_target_properties(sopranobackend PROPERTIES PREFIX ${prefix})

target_link_libraries(sopranobackend
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_INDEX_LIBRARIES}
  ${SOPRANO_CLIENT_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${CLUCENE_LIBRARY}
)

install(TARGETS sopranobackend LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)
