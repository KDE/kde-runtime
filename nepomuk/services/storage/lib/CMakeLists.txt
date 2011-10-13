project(libdatamanagement)

set(datamanagement_SRC
  simpleresource.cpp
  simpleresourcegraph.cpp
  genericdatamanagementjob.cpp
  createresourcejob.cpp
  describeresourcesjob.cpp
  storeresourcesjob.cpp
  dbustypes.cpp
  datamanagement.cpp
  resourcewatcher.cpp
  abstracttimeoutdbusinterface.cpp
  datamanagementinterface.cpp
  kdbusconnectionpool.cpp
)

qt4_add_dbus_interface(datamanagement_SRC
  ../../../interfaces/org.kde.nepomuk.ResourceWatcherConnection.xml
  resourcewatcherconnectioninterface)

qt4_add_dbus_interface(datamanagement_SRC
  ../../../interfaces/org.kde.nepomuk.ResourceWatcher.xml
  resourcewatchermanagerinterface)

kde4_add_library(nepomukdatamanagement SHARED
  ${datamanagement_SRC})

target_link_libraries(nepomukdatamanagement
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${KDE4_KDECORE_LIBS})

install(TARGETS nepomukdatamanagement ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  simpleresource.h
  simpleresourcegraph.h
  datamanagement.h
  createresourcejob.h
  describeresourcesjob.h
  storeresourcesjob.h
  resourcewatcher.h
  nepomukdatamanagement_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/nepomuk
  COMPONENT Devel)

# API docs
find_package(Doxygen)

if(DOXYGEN_EXECUTABLE)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  add_custom_target(
    dms-apidox
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile)
endif(DOXYGEN_EXECUTABLE)
