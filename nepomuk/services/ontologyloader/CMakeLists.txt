project(ontologyloader)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(ontologyloader_SRCS
  ontologyloader.cpp
  ontologymanagermodel.cpp
  )

kde4_add_plugin(nepomukontologyloader ${ontologyloader_SRCS})

target_link_libraries(nepomukontologyloader
  ${SOPRANO_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukontologyloader.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukontologyloader
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
