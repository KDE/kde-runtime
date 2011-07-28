project(nepomukwritebackservices)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)


include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
 )

set( writebackservice_SCR
writebackservice.cpp
writebackjob.cpp
)
kde4_add_plugin(nepomukwritebackservice ${writebackservice_SCR})


target_link_libraries(nepomukwritebackservice
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  nepomukwriteback 
  nepomukdatamanagement
  )


install(
  FILES nepomukwritebackservice.desktop 
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukwritebackservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
