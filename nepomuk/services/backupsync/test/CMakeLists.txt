project(nepomukbackupsync_tests)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include( SopranoAddOntology )
include( KDE4Defaults )
include( NepomukTestLibMacros )

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${KDE4_INCLUDES}
  ${KDE4_KIO_INCLUDES}
  ${QT_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

#--------- Adaptors ---------#

set( backuptest_SRCS backuptests.cpp )

soprano_add_ontology(backuptest_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../ontology/identification.trig
  "backupsync"
  "Nepomuk::Vocabulary"
  "trig")

qt4_add_dbus_interface(backuptest_SRCS ../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml backupmanager)

kde4_add_executable( nepomukbackuptests ${backuptest_SRCS} )
add_nepomuk_test(nepomukbackuptests ${CMAKE_CURRENT_BINARY_DIR}/nepomukbackuptests)

target_link_libraries( nepomukbackuptests
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTTEST_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  kdecore
  nepomuksync
  nepomuktest
)

