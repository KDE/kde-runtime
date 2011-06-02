project(nepomukwritebackservice)

find_package(KDE4 REQUIRED)
find_package(Nepomuk REQUIRED)
find_package(Taglib)
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

include (KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${TAGLIB_INCLUDES}
 )

set( SRCS
mywritebackplugin.cpp
writebackplugin.cpp
writebackservice.cpp
)

kde4_add_plugin(nepomukwritebackservice ${SRCS})

target_link_libraries(nepomukwritebackservice
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${TAGLIB_LIBRARIES})

install(
  FILES nepomukwritebackservice.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS nepomukwritebackservice
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
