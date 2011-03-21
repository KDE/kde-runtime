project(nepomukdatamanagement)

set(KDE_MIN_VERSION "4.5.80")  # for the < 4.2 macro
find_package(KDE4 4.5.80 REQUIRED)
find_package(Nepomuk REQUIRED)

include(KDE4Defaults)
include(SopranoAddOntology)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/lib
)

set( datamanagement_BIN_SRCS
  transactionmodel.cpp
  datamanagementservice.cpp
  datamanagementmodel.cpp
  datamanagementadaptor.cpp
  datamanagementcommand.cpp
  classandpropertytree.cpp
  resourcemerger.cpp
  resourceidentifier.cpp
  removablemediamodel.cpp
)

kde4_add_plugin(nepomukdatamanagementservice ${datamanagement_BIN_SRCS})

target_link_libraries(nepomukdatamanagementservice
  ${QT_QTGUI_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  nepomuksync
  nepomukdatamanagement
)

install(TARGETS nepomukdatamanagementservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES nepomukdatamanagementservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

add_subdirectory(lib)
add_subdirectory(test)
