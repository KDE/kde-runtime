project(nepomuk_interfaces)

install(FILES
  org.kde.NepomukServer.xml
  org.kde.nepomuk.ServiceControl.xml
  org.kde.nepomuk.ServiceManager.xml
  org.kde.nepomuk.Strigi.xml
  org.kde.nepomuk.OntologyManager.xml
  org.kde.nepomuk.Storage.xml
  org.kde.nepomuk.QueryService.xml
  org.kde.nepomuk.Query.xml
  org.kde.nepomuk.RemovableStorage.xml
  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
  )
