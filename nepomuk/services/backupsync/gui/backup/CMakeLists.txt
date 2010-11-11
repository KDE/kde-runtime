project(nepomukbackup_gui)

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
  ${CMAKE_CURRENT_BUILD_DIR}
  )

set( NepomukBackup_GUI_SRCS 
  backupwizardpages.cpp
  backupwizard.cpp
  mergeconflictdelegate.cpp
  main.cpp
    identifiermodel.cpp
    identifiermodeltree.cpp
    identifierwidget.cpp
    ../../service/dbusoperators.cpp
)

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
  PROPERTIES INCLUDE "../../service/dbusoperators.h")

qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
                        identifierinterface )

qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.merger.xml
                        mergerinterface )


#----- DBus interfaces --------

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QList")

set_source_files_properties(
  ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  PROPERTIES INCLUDE "QtCore/QString")

qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
                        syncmanagerinterface )
qt4_add_dbus_interface( NepomukBackup_GUI_SRCS
                        ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
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


kde4_add_executable(nepomukbackup ${NepomukBackup_GUI_SRCS})


target_link_libraries(nepomukbackup
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(TARGETS nepomukbackup
        DESTINATION ${BIN_INSTALL_DIR})

