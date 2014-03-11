project(kioslave-nepomuk-tags)

include_directories(
  ${QT_INCLUDES}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

########### next target ###############

set(kio_nepomuk_tags_PART_SRCS
  kio_tags.cpp
)

#qt5_add_dbus_interface(kio_timeline_PART_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.ServiceControl.xml nepomukservicecontrolinterface)

add_library(kio_tags MODULE ${kio_nepomuk_tags_PART_SRCS})

target_link_libraries(kio_tags
  KF5::KIOWidgets
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
)

install(TARGETS kio_tags DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES tags.protocol  DESTINATION  ${SERVICES_INSTALL_DIR})
