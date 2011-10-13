project(nepomuk_interfaces)

install(FILES
  org.kde.NepomukServer.xml
  org.kde.nepomuk.ServiceControl.xml
  org.kde.nepomuk.ServiceManager.xml
  org.kde.nepomuk.FileIndexer.xml
  org.kde.nepomuk.OntologyManager.xml
  org.kde.nepomuk.Storage.xml
  org.kde.nepomuk.QueryService.xml
  org.kde.nepomuk.Query.xml
  org.kde.nepomuk.BackupSync.xml
  org.kde.nepomuk.services.nepomukbackupsync.backupmanager.xml
  org.kde.nepomuk.services.nepomukbackupsync.syncmanager.xml
  org.kde.nepomuk.DataManagement.xml
  org.kde.nepomuk.ResourceWatcher.xml
  org.kde.nepomuk.ResourceWatcherConnection.xml
  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
  )
