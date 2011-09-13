project(nepomuk_common)

set(nepomukcommon_SRCS
  fileexcludefilters.cpp
  regexpcache.cpp
  removablemediacache.cpp
)

kde4_add_library(nepomukcommon SHARED ${nepomukcommon_SRCS})

target_link_libraries(nepomukcommon
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  ${KDE4_SOLID_LIBRARY}
  ${NEPOMUK_LIBRARIES}
  )

install(TARGETS nepomukcommon EXPORT ${INSTALL_TARGETS_DEFAULT_ARGS})

# Note: no headers installed since this is a private shared lib
