project(nepomukbackup_gui_test)

find_package(KDE4 REQUIRED)

include(KDE4Defaults)

include_directories(
  ${QT_INCLUDES}
  ${KDE4_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${NEPOMUK_INCLUDE_DIR}
  ${libnepomuksync_SOURCE_DIR}
  ${CMAKE_CURRENT_BUILD_DIR}
  )

set( NepomukBackup_GUI_TEST_SRCS 
  main.cpp
  testwidget.cpp
  ../identifiermodel.cpp
  ../identifiermodeltree.cpp
  ../filesystemtree.cpp
  ../mergeconflictdelegate.cpp
)

kde4_add_executable(nepomukbackupguitest ${NepomukBackup_GUI_TEST_SRCS})

target_link_libraries(nepomukbackupguitest
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  )
