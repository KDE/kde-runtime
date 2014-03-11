project(kcm_nepomuk)

# Nepomuk KCM
# -----------------------------
include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  )

set(kcmnepomuk_SRCS
  nepomukserverkcm.cpp
  fileexcludefilters.cpp
  folderselectionmodel.cpp
  indexfolderselectiondialog.cpp
  excludefilterselectiondialog.cpp
  detailswidget.cpp
  removablemediacache.cpp
  )

qt5_add_dbus_interface(kcmnepomuk_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.NepomukServer.xml nepomukserverinterface)
qt5_add_dbus_interface(kcmnepomuk_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.ServiceControl.xml servicecontrol)
qt5_add_dbus_interface(kcmnepomuk_SRCS ${NEPOMUK_CORE_DBUS_INTERFACES_DIR}/org.kde.nepomuk.FileIndexer.xml fileindexerinterface)
qt5_add_dbus_interface(kcmnepomuk_SRCS ./org.freedesktop.Akonadi.NepomukFeeder.xml akonadifeederinterface)

qt5_wrap_ui(kcmnepomuk_SRCS
  nepomukconfigwidget.ui
  indexfolderselectionwidget.ui
  excludefilterselectiondialog.ui)

add_library(kcm_nepomuk MODULE ${kcmnepomuk_SRCS})

target_link_libraries(kcm_nepomuk
  KF5::KDE4Support
  KF5::KIOWidgets
  ${NEPOMUK_CORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${KDE4_SOLID_LIBRARY}
  )
# -----------------------------


# Installation
# -----------------------------
install(
  FILES kcm_nepomuk.desktop desktop-search.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS kcm_nepomuk
  DESTINATION ${PLUGIN_INSTALL_DIR})
# -----------------------------
