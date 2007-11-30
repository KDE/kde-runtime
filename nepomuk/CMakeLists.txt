project(nepomuk)

find_package(Soprano REQUIRED)

if(SopranoIndex_FOUND)
  set(HAVE_SOPRANO_INDEX "yes")
endif(SopranoIndex_FOUND)

# optional Strigi backend
# -----------------------------
find_package(CLucene)
if(CLucene_FOUND AND SopranoIndex_FOUND)
  set(HAVE_STRIGI_SOPRANO_BACKEND "yes")
  macro_optional_add_subdirectory(strigibackend)
endif(CLucene_FOUND AND SopranoIndex_FOUND)
# -----------------------------

configure_file(nepomukserver-config.h.cmake ${nepomuk_BINARY_DIR}/nepomukserver-config.h)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomuk_BINARY_DIR}
  )

# NepomukServer
# -----------------------------

# Server configuration
kde4_add_kcfg_files(nepomukserversettings_SRCS nepomukserversettings.kcfgc)

set(nepomukserver_SRCS
  nepomukcore.cpp
  nepomukserver.cpp
  repository.cpp
  strigicontroller.cpp
  #  nepomuksearchagent.cpp
  ${nepomukserversettings_SRCS}
  )

qt4_add_dbus_adaptor(nepomukserver_SRCS org.kde.NepomukServer.xml nepomukserver.h Nepomuk::Server)

kde4_add_plugin(kded_nepomukserver ${nepomukserver_SRCS})

target_link_libraries(kded_nepomukserver
  ${STRIGI_STRIGIQTDBUSCLIENT_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_SERVER_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  )

if(SopranoIndex_FOUND)
  target_link_libraries(kded_nepomukserver ${SOPRANO_INDEX_LIBRARIES})
endif(SopranoIndex_FOUND)
# -----------------------------

set(kcmnepomuk_SRCS
  strigicontroller.cpp
  nepomukserverkcm.cpp
  ${nepomukserversettings_SRCS}
  )


# Nepomuk KCM
# -----------------------------
qt4_add_dbus_interface(kcmnepomuk_SRCS org.kde.NepomukServer.xml nepomukserverinterface)

kde4_add_ui_files(kcmnepomuk_SRCS
  nepomukconfigwidget.ui)

kde4_add_plugin(kcm_nepomuk ${kcmnepomuk_SRCS})

target_link_libraries(kcm_nepomuk
  ${STRIGI_STRIGIQTDBUSCLIENT_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  )
# -----------------------------


# Installation
# -----------------------------
install(
  FILES nepomukserver.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(
  FILES kcm_nepomuk.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS kcm_nepomuk kded_nepomukserver
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
