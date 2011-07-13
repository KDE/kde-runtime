project(nepomukbackup_gui)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${libnepomuksync_SOURCE_DIR}
  ${CMAKE_CURRENT_BUILD_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../service/
  )

set( NepomukBackup_GUI_SRCS
  backupwizardpages.cpp
  backupwizard.cpp
  mergeconflictdelegate.cpp
  main.cpp
  identifiermodel.cpp
  identifiermodeltree.cpp
  identifierwidget.cpp
  filesystemtree.cpp

  # Restoration code
  identifier.cpp
  merger.cpp
  changelogmerger.cpp
  syncfileidentifier.cpp
  resourcelog.cpp

   # Code required by restoration
  ../service/tools.cpp
  ../service/changelog.cpp
  ../service/changelogrecord.cpp
  ../service/identificationset.cpp
  ../service/syncfile.cpp
)

#----- DBus interfaces --------

set_source_files_properties(
  ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QList")
set_source_files_properties(
  ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QString")

qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
                        syncmanagerinterface )
qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
                        backupmanagerinterface )

#------UI Interface files ------

kde4_add_ui_files( NepomukBackup_GUI_SRCS
  intropage.ui
  backuppage.ui
  backupsettingspage.ui
  mergeconflictwidget.ui
  restoreselection.ui
  restorefinal.ui
  errorpage.ui
)

#--------- Ontologies -------#
soprano_add_ontology(NepomukBackup_GUI_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../../../ontologies/nrio.trig
  "NRIO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_executable(nepomukbackup ${NepomukBackup_GUI_SRCS})

target_link_libraries(nepomukbackup
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  nepomuksync
  )

install(TARGETS nepomukbackup ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(PROGRAMS nepomukbackup.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR})
