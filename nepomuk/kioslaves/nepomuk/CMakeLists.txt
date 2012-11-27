include_directories(
  ${KDE4_INCLUDES}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${nepomuk_kio_slaves_SOURCE_DIR}/common
)

set(kio_nepomuk_PART_SRCS 
  kio_nepomuk.cpp
  resourcepagegenerator.cpp)

kde4_add_plugin(kio_nepomuk ${kio_nepomuk_PART_SRCS})

target_link_libraries(kio_nepomuk
  ${KDE4_KIO_LIBS}
  ${KDE4_SOLID_LIBS}
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES})

install(TARGETS kio_nepomuk DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES nepomuk.protocol DESTINATION ${SERVICES_INSTALL_DIR})
