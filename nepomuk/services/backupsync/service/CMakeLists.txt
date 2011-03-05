project(nepomukbackupsync)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)
#find_package(nepomuksync REQUIRED)

include(SopranoAddOntology)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300106)

include (KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${libnepomuksync_SOURCE_DIR}
  ../lib
  )

set( BackupSyncService_SRCS
  identifier.cpp
  merger.cpp
  logstorage.cpp
  backupsyncservice.cpp
  diffgenerator.cpp
  syncmanager.cpp
  backupmanager.cpp
  dbusoperators.cpp
  changelogmerger.cpp
  syncfileidentifier.cpp
  tools.cpp
  resourcelog.cpp
  changelog.cpp
  changelogrecord.cpp
  identificationset.cpp
  syncfile.cpp
)

#--------- Adaptors ---------#

qt4_add_dbus_adaptor( BackupSyncService_SRCS
                      ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
                      identifier.h Nepomuk::Identifier )
qt4_add_dbus_adaptor( BackupSyncService_SRCS
                      ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.merger.xml
                      merger.h Nepomuk::Merger )
qt4_add_dbus_adaptor( BackupSyncService_SRCS
                      ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
                      backupmanager.h Nepomuk::BackupManager )
qt4_add_dbus_adaptor( BackupSyncService_SRCS
                      ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
                      syncmanager.h Nepomuk::SyncManager )

#--------- Ontologies -------#
soprano_add_ontology(BackupSyncService_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../../../ontologies/nrio.trig
  "NRIO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_plugin(nepomukbackupsync ${BackupSyncService_SRCS})

target_link_libraries(nepomukbackupsync
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  nepomuksync
  ${SOPRANO_LIBRARIES}
  )

install(
  FILES nepomukbackupsync.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukbackupsync
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
