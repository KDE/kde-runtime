project(nepomuk_common)

set(nepomukcommon_SRCS
  fileexcludefilters.cpp
  regexpcache.cpp
)

soprano_add_ontology(nepomukcommon_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig"
  VISIBILITY "nepomukcommon")
soprano_add_ontology(nepomukcommon_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig"
  VISIBILITY "nepomukcommon")
soprano_add_ontology(nepomukcommon_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/pimo/pimo.trig
  "PIMO"
  "Nepomuk::Vocabulary"
  "trig"
  VISIBILITY "nepomukcommon")
soprano_add_ontology(nepomukcommon_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nuao.trig
  "NUAO"
  "Nepomuk::Vocabulary"
  "trig"
  VISIBILITY "nepomukcommon")

kde4_add_library(nepomukcommon SHARED ${nepomukcommon_SRCS})

target_link_libraries(nepomukcommon
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  ${NEPOMUK_LIBRARIES}
  )

install(TARGETS nepomukcommon EXPORT ${INSTALL_TARGETS_DEFAULT_ARGS})

# Note: no headers installed since this is a private shared lib
