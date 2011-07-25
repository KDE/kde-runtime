project(nepomukwritebackplugins)

find_package(Kexiv2)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)


include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${KEXIV2_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
 )

set( writebackplugins_SCR
Exiv2WritebackPlugin.cpp
)
kde4_add_plugin(nepomuk_writeback_exiv2 ${writebackplugins_SCR})


target_link_libraries(nepomuk_writeback_exiv2
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KEXIV2_LIBRARIES}
  nepomukwriteback  
)


install(
  FILES nepomuk_writeback_exiv2.desktop 
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomuk_writeback_exiv2
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
 
