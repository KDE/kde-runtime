project(runtime_kimgio)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

##################################
# WebP image IO support

find_library(FOUND_WEBP webp)
if ( FOUND_WEBP )
  set(kimg_webp_LIB_SRCS webp.cpp)
  kde4_add_plugin(kimg_webp ${kimg_webp_LIB_SRCS})
  target_link_libraries(kimg_webp ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} webp)

  install(TARGETS kimg_webp  DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/imageformats/)
  install(FILES webp.desktop DESTINATION ${SERVICES_INSTALL_DIR}/qimageioplugins/)

  find_package(SharedMimeInfo REQUIRED)
  install(FILES webp.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
  update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif()

