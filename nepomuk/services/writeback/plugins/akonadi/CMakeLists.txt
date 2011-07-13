project(nepomukwritebackplugins)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)
find_package (KdepimLibs REQUIRED)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)


include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${Akonadi_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
 )

set( writebackplugins_SCR
myakonadiplugin.cpp
)
kde4_add_plugin(nepomuk_writeback_akonadi ${writebackplugins_SCR})


target_link_libraries(nepomuk_writeback_akonadi
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KABC_LIBS}
  ${Kdepimlibs_LIBS} 
  ${KDE4_AKONADI_LIBS} 
  ${KDE4_KDEUI_LIBS} 
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDE4_KPIMUTILS_LIBS} 
  ${KDE4_AKONADI_KMIME_LIBS}
  nepomukwriteback  
)


install(
  FILES nepomuk_writeback_akonadi.desktop 
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomuk_writeback_akonadi
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
 
