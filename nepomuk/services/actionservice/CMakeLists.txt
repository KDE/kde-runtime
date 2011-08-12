project(nepomukactionservice)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300101)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(nepomukactions_SRCS
  resourceactionplugin.cpp
)
kde4_add_library(nepomukactions
  ${nepomukactions_SRCS}
)
target_link_libraries(nepomukactions
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  )
install(TARGETS nepomukactions
  DESTINATION ${LIB_INSTALL_DIR})
install(FILES
  resourceactionplugin.h
  nepomukactions_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/nepomuk
  COMPONENT Devel)
install(FILES
  nepomukactionplugin.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR})



set(nepomukactionservice_SRCS
  actionservice.cpp
  )
kde4_add_plugin(nepomukactionservice ${nepomukactionservice_SRCS})
target_link_libraries(nepomukactionservice
  nepomukactions
  ${SOPRANO_CLIENT_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  )
install(
  FILES nepomukactionservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
install(
  TARGETS nepomukactionservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
