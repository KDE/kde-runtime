project(kcm_nepomuk)

# Nepomuk KCM
# -----------------------------
include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${nepomukcommon_BINARY_DIR}
  )

set(kcmnepomuk_SRCS
  ../common/strigiconfigfile.cpp
  nepomukserverkcm.cpp
  )

qt4_add_dbus_interface(kcmnepomuk_SRCS ../interfaces/org.kde.NepomukServer.xml nepomukserverinterface)

kde4_add_kcfg_files(kcmnepomuk_SRCS ../server/nepomukserversettings.kcfgc)

kde4_add_ui_files(kcmnepomuk_SRCS
  nepomukconfigwidget.ui)

kde4_add_plugin(kcm_nepomuk ${kcmnepomuk_SRCS})

target_link_libraries(kcm_nepomuk
  ${STRIGI_STRIGIQTDBUSCLIENT_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  )
# -----------------------------


# Installation
# -----------------------------
install(
  FILES kcm_nepomuk.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS kcm_nepomuk
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
