project(strigiservice)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomukcommon_BINARY_DIR}
  )

set(strigiservice_SRCS
  strigiservice.cpp
  strigicontroller.cpp
  ../../common/strigiconfigfile.cpp
  )

kde4_add_plugin(nepomukstrigiservice ${strigiservice_SRCS})

target_link_libraries(nepomukstrigiservice
  ${STRIGI_STRIGIQTDBUSCLIENT_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(
  FILES nepomukstrigiservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukstrigiservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
