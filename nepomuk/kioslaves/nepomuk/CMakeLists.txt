include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${nepomuk_kio_slaves_SOURCE_DIR}/common
)

set(kio_nepomuk_PART_SRCS 
  kio_nepomuk.cpp
  resourcepagegenerator.cpp)

add_library(kio_nepomuk MODULE ${kio_nepomuk_PART_SRCS})

target_link_libraries(kio_nepomuk
  KF5::KIOWidgets
  KF5::Solid
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES})

install(TARGETS kio_nepomuk DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES nepomuk.protocol DESTINATION ${SERVICES_INSTALL_DIR})
