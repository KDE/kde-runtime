project(nepomukbackupsync_gui)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)

include(SopranoAddOntology)
include (KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${libnepomuksync_SOURCE_DIR}
  )

set( BackupSyncService_GUI_SRCS 
  main.cpp
  controlwidget.cpp
 ../../service/dbusoperators.cpp
  )

#--------- Ontologies -------#
soprano_add_ontology(BackupSyncService_GUI_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(BackupSyncService_GUI_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nao.trig
  "NAO"
  "Nepomuk::Vocabulary"
  "trig")

soprano_add_ontology(BackupSyncService_GUI_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")

#----- DBus interfaces --------

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QList")

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QString")

qt4_add_dbus_interface( BackupSyncService_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
                        syncmanagerinterface )
qt4_add_dbus_interface( BackupSyncService_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
                        backupmanagerinterface )

#------UI Interface files ------
kde4_add_ui_files( BackupSyncService_GUI_SRCS
  controlwidget.ui)

kde4_add_ui_files( BackupSyncService_GUI_SRCS
  ../common/mergerwidget.ui )

kde4_add_executable(NepomukBackupSync ${BackupSyncService_GUI_SRCS})


target_link_libraries(NepomukBackupSync
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  NepomukBackupSyncGuiCommon
  )

install(TARGETS NepomukBackupSync
        DESTINATION ${BIN_INSTALL_DIR})
