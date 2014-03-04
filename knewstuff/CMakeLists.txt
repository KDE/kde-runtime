
add_executable(khotnewstuff4 khotnewstuff.cpp)
target_link_libraries(khotnewstuff4 KF5::NewStuff KF5::KDE4Support)

install(TARGETS khotnewstuff4 ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES khotnewstuff.knsrc DESTINATION  ${CONFIG_INSTALL_DIR} )

if(LIBATTICA_FOUND)
  include_directories(${LIBATTICA_INCLUDE_DIR})

  add_executable(khotnewstuff-upload khotnewstuff_upload.cpp)
  target_link_libraries(khotnewstuff-upload KF5::KDE4Support)

  install(TARGETS khotnewstuff-upload ${INSTALL_TARGETS_DEFAULT_ARGS} )
  install( FILES khotnewstuff_upload.knsrc DESTINATION  ${CONFIG_INSTALL_DIR} )
endif(LIBATTICA_FOUND)
