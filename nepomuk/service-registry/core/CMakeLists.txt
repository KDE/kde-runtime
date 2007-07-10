
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KNEPOMUK_INCLUDES}
)

set( nepomukregcore_LIB_SRCS
  core.cpp
  service.cpp
  dbusbackend.cpp
  dbusbackendinterface.cpp
  servicewaiter.cpp
)


kde4_add_library(nepomukregcore STATIC ${nepomukregcore_LIB_SRCS})

target_link_libraries(nepomukregcore
  ${NEPOMUK_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBS}
)
