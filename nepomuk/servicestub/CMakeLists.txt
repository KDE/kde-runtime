project(servicestub)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_CURRENT_BUILD_DIR}
  )

set(servicestub_SRCS
  main.cpp
  servicecontrol.cpp
)

qt4_add_dbus_adaptor(servicestub_SRCS
  ../interfaces/org.kde.nepomuk.ServiceControl.xml
  servicecontrol.h
  Nepomuk::ServiceControl)

kde4_add_executable(nepomukservicestub
  ${servicestub_SRCS}
)

target_link_libraries(nepomukservicestub
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  TARGETS nepomukservicestub
  DESTINATION ${BIN_INSTALL_DIR})
