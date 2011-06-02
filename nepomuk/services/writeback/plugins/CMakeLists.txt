project(nepomukwritebackplugins)

find_package(Taglib)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)


include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${TAGLIB_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
 )

set( writebackplugins_SCR
mywritebackplugin.cpp
)
kde4_add_plugin(nepomukwritebackplugins ${writebackplugins_SCR})


target_link_libraries(nepomukwritebackplugins
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${TAGLIB_LIBRARIES}
  nepomukwriteback  
)


install(
  FILES nepomuk_writeback_taglib.desktop 
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukwritebackplugins
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
