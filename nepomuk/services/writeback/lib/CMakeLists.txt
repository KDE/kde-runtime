project(nepomukwriteback)

add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)


include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
 )

set( libwriteback_SRCS
writebackplugin.cpp
)
kde4_add_library(nepomukwriteback SHARED ${libwriteback_SRCS})


target_link_libraries(nepomukwriteback
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  nepomukwriteback
)


install(
  FILES nepomukwritebackplugin.desktop 
  DESTINATION ${SERVICETYPES_INSTALL_DIR})

install(
  TARGETS nepomukwriteback
  DESTINATION ${LIB_INSTALL_DIR})
# -----------------------------
