project(kioslave-nepomuk-timeline)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${KDE4_KIO_INCLUDES}
  ${NEPOMUK_INCLUDE_DIR}
)

########### next target ###############

set(kio_timeline_PART_SRCS kio_timeline.cpp)

kde4_add_plugin(kio_timeline ${kio_timeline_PART_SRCS})

target_link_libraries(kio_timeline
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
)

install(TARGETS kio_timeline  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES timeline.protocol  DESTINATION  ${SERVICES_INSTALL_DIR})
