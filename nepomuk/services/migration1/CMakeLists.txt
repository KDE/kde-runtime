project(migration1)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300102)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(migration1_SRCS
  migration1.cpp
  )

kde4_add_plugin(nepomukmigration1 ${migration1_SRCS})

target_link_libraries(nepomukmigration1
  ${SOPRANO_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukmigration1.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukmigration1
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
