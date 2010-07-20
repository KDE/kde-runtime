project(nepomukstrigiservice)

include(SopranoAddOntology)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${STRIGI_INCLUDE_DIR}
  ${nepomukstrigiservice_BUILD_DIR}
  )

# Check if Strigi has the FileInputStream::open method which was introduced between versions 0.7.2 and 0.7.3
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${STRIGI_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${STRIGI_STREAMS_LIBRARY})
CHECK_CXX_SOURCE_COMPILES(
  "#include <strigi/fileinputstream.h>\nint main(int ac,char* av[]) { Strigi::FileInputStream::open(\"dummy\"); }"
  STRIGI_HAS_FILEINPUTSTREAM_OPEN
)
if(${STRIGI_HAS_FILEINPUTSTREAM_OPEN})
  add_definitions(-DSTRIGI_HAS_FILEINPUTSTREAM_OPEN)
endif(${STRIGI_HAS_FILEINPUTSTREAM_OPEN})

set(strigiservice_SRCS
  strigiservice.cpp
  indexscheduler.cpp
  strigiserviceconfig.cpp
  eventmonitor.cpp
  nepomukindexwriter.cpp
  nepomukindexfeeder.cpp
  util.cpp
  nepomukindexer.cpp
  )

soprano_add_ontology(strigiservice_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(strigiservice_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")

qt4_add_dbus_adaptor(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.Strigi.xml strigiservice.h Nepomuk::StrigiService )

qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.RemovableStorage.xml removablestorageserviceinterface)
qt4_add_dbus_interface(strigiservice_SRCS ../../interfaces/org.kde.nepomuk.FileWatch.xml filewatchserviceinterface)

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
  ${STRIGI_STREAMANALYZER_LIBRARY}
  ${STRIGI_STREAMS_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${KDE4_KIDLETIME_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  FILES nepomukstrigiservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  FILES nepomukstrigiservice.notifyrc
  DESTINATION ${DATA_INSTALL_DIR}/nepomukstrigiservice)

install(
  TARGETS nepomukstrigiservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
