
include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${NEPOMUK_INCLUDES}
  ${nepomukquery_SOURCE_DIR}
  ${nepomukqueryclient_SOURCE_DIR}
)

set( nepomukqueryservice_test_SRCS
  queryservicetester.cpp
  queryclient.cpp
)

kde4_add_executable(queryservicetester ${nepomukqueryservice_test_SRCS})
target_link_libraries(queryservicetester
  nepomukqueryclient
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  )
