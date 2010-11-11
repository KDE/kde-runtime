# project(nepomukbackupsync_gui_common)
# 
# find_package(KDE4 REQUIRED)
# find_package(Nepomuk REQUIRED)
# 
# include(SopranoAddOntology)
# include (KDE4Defaults)
# 
# include_directories(
#   ${QT_INCLUDES}
#   ${KDE4_INCLUDES}
#   ${SOPRANO_INCLUDE_DIR}
#   ${CMAKE_SOURCE_DIR}
#   ${NEPOMUK_INCLUDE_DIR}
#   ${libnepomuksync_SOURCE_DIR}
#   )
#  
# set( NEPOMUK_BACKUPSYNC_GUI_COMMON_SRCS
#     identifiermodel.cpp
#     identifiermodeltree.cpp
#     identifierwidget.cpp
#     mergermodel.cpp
#     mergerwidget.cpp
#     resolutiondialog.cpp
#     ../../service/dbusoperators.cpp
# )
# 
# set_source_files_properties(
#   ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
#   PROPERTIES INCLUDE "../../service/dbusoperators.h")
# 
# qt4_add_dbus_interface( NEPOMUK_BACKUPSYNC_GUI_COMMON_SRCS
#                         ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.identifier.xml
#                         identifierinterface )
# qt4_add_dbus_interface( NEPOMUK_BACKUPSYNC_GUI_COMMON_SRCS
#                         ../../interfaces/org.kde.nepomuk.services.nepomukbackupsync.merger.xml
#                         mergerinterface )
# 
# kde4_add_ui_files( NEPOMUK_BACKUPSYNC_GUI_COMMON_SRCS
#   mergerwidget.ui)
# 
# kde4_add_library( NepomukBackupSyncGuiCommon STATIC ${NEPOMUK_BACKUPSYNC_GUI_COMMON_SRCS} )
# 
# target_link_libraries( NepomukBackupSyncGuiCommon
#   ${SOPRANO_LIBRARIES}
#   ${NEPOMUK_LIBRARIES}
#   ${KDE4_KDEUI_LIBS}
#   ${KDE4_KIO_LIBS}
#   nepomuksync
# )
# 
# install(TARGETS NepomukBackupSyncGuiCommon EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
