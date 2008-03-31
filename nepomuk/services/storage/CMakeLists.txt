project(storage)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

configure_file(nepomukstorage-config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/nepomukstorage-config.h)

set(storage_SRCS
  storage.cpp
  nepomukcore.cpp
  modelcopyjob.cpp
  repository.cpp
  )

kde4_add_plugin(nepomukstorage ${storage_SRCS})

target_link_libraries(nepomukstorage
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

if(SopranoIndex_FOUND)
  target_link_libraries(nepomukstorage ${SOPRANO_INDEX_LIBRARIES})
endif(SopranoIndex_FOUND)

install(
  FILES nepomukstorage.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukstorage
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
