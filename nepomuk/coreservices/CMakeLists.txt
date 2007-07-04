
include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDES}
)

set( SRCS
  cache.cpp
  uniqueurigenerator.cpp
  querydefinition.cpp     	
  sopranordfrepository.cpp
  coreservices.cpp
  dbusinterface.cpp
)

kde4_automoc(${SRCS})

kde4_add_executable(nepomukcoreservices ${SRCS} main.cpp)

target_link_libraries(nepomukcoreservices
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
)

install(FILES nepomuk-coreservices.desktop DESTINATION ${SERVICES_INSTALL_DIR}/nepomuk)

install(TARGETS nepomukcoreservices DESTINATION ${BIN_INSTALL_DIR} )
