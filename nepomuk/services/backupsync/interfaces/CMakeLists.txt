project(nepomuk_backupsync_interfaces)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

install(FILES
  org.kde.nepomuk.BackupSync.xml
  org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
  org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
  org.kde.nepomuk.services.nepomukbackupsync.merger.xml
  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
  )
