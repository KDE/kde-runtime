project(kcm_nepomuk)

# Nepomuk KCM
# -----------------------------
include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  )

set(kcmnepomuk_SRCS
  nepomukserverkcm.cpp
  folderselectionmodel.cpp
  indexfolderselectiondialog.cpp
  )

qt4_add_dbus_interface(kcmnepomuk_SRCS ../interfaces/org.kde.NepomukServer.xml nepomukserverinterface)
qt4_add_dbus_interface(kcmnepomuk_SRCS ../interfaces/org.kde.nepomuk.ServiceControl.xml nepomukservicecontrolinterface)
qt4_add_dbus_interface(kcmnepomuk_SRCS ../interfaces/org.kde.nepomuk.Strigi.xml strigiserviceinterface)

kde4_add_ui_files(kcmnepomuk_SRCS
  nepomukconfigwidget.ui
  indexfolderselectionwidget.ui)

soprano_add_ontology(kcmnepomuk_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  "NIE"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kcmnepomuk_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
  "NFO"
  "Nepomuk::Vocabulary"
  "trig")
soprano_add_ontology(kcmnepomuk_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nuao.trig
  "NUAO"
  "Nepomuk::Vocabulary"
  "trig")

kde4_add_plugin(kcm_nepomuk ${kcmnepomuk_SRCS})

target_link_libraries(kcm_nepomuk
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${SOPRANO_LIBRARIES}
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
