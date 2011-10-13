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
  priority.cpp
)

qt4_add_dbus_adaptor(servicestub_SRCS
  ../interfaces/org.kde.nepomuk.ServiceControl.xml
  servicecontrol.h
  Nepomuk::ServiceControl)

kde4_add_executable(nepomukservicestub
  ${servicestub_SRCS}
)

if (Q_WS_MAC)
    set_target_properties(nepomukservicestub PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
    set_target_properties(nepomukservicestub PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.nepomuk.servicestub")
    set_target_properties(nepomukservicestub PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Nepomuk Service Stub")
endif (Q_WS_MAC)


target_link_libraries(nepomukservicestub
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )

install(
  TARGETS nepomukservicestub
  ${INSTALL_TARGETS_DEFAULT_ARGS})
