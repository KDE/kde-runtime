project(strigi_nepomuk_indexer)

include(SopranoAddOntology)

include_directories( 
  ${STRIGI_INCLUDE_DIR}
  ${SOPRANO_INCLUDE_DIR}
)

set( strigi_nepomuk_indexer_SRCS
  nepomukindexmanager.cpp
  nepomukindexreader.cpp
  nepomukindexwriter.cpp
  util.cpp
)

soprano_add_ontology(strigi_nepomuk_indexer_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nrl.trig
  "NRL"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(strigi_nepomuk_indexer_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(strigi_nepomuk_indexer_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")

if(MSVC)
  # add some define to link correctly against clucene
  add_definitions(-Zc:wchar_t-)
endif(MSVC)

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

kde4_add_library(nepomukbackend MODULE ${strigi_nepomuk_indexer_SRCS})

set_target_properties(nepomukbackend PROPERTIES PREFIX ${prefix})

target_link_libraries(nepomukbackend
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_CLIENT_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KDECORE_LIBS}
)

install(TARGETS nepomukbackend LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)
