project(knepregcore)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${knepregistry_SOURCE_DIR}
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KNEPOMUK_INCLUDES}
)

set( knepregcore_LIB_SRCS
  core.cpp
  service.cpp
  dbusbackend.cpp
  dbusbackendinterface.cpp
  servicewaiter.cpp
)

kde4_automoc(${knepregcore_LIB_SRCS})

kde4_add_library(knepregcore STATIC ${knepregcore_LIB_SRCS})

target_link_libraries(knepregcore
  ${KNEPOMUK_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBS}
)
