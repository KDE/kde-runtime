project(libdatamanagement)

set(datamanagement_SRC
  simpleresource.cpp
  simpleresourcegraph.cpp
  genericdatamanagementjob.cpp
  createresourcejob.cpp
  describeresourcesjob.cpp
  dbustypes.cpp
  datamanagement.cpp
)

set(datamanagementservice_dbus_desc ${CMAKE_CURRENT_SOURCE_DIR}/../org.kde.nepomuk.DataManagement.xml)

set_source_files_properties(
  ${datamanagementservice_dbus_desc}
  PROPERTIES
  INCLUDE "dbustypes.h")

qt4_add_dbus_interface(datamanagement_SRC
  ${datamanagementservice_dbus_desc}
  datamanagementinterface)

kde4_add_library(nepomukdatamanagement SHARED
  ${datamanagement_SRC})

target_link_libraries(nepomukdatamanagement
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${KDE4_KDECORE_LIBS})

install(TARGETS nepomukdatamanagement DESTINATION ${LIB_INSTALL_DIR})
install(FILES
  simpleresource.h
  simpleresourcegraph.h
  datamanagement.h
  createresourcejob.h
  describeresourcesjob.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/nepomuk)
