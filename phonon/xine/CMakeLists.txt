add_subdirectory(kcm)

include_directories(${XINE_INCLUDE_DIR})

# xine expects off_t to be 64bits
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)

set(phonon_xine_PART_SRCS
   sinknode.cpp
   sourcenode.cpp
   xinethread.cpp
   nullsink.cpp
    xineengine.cpp
    xinestream.cpp
    audioport.cpp
    abstractaudiooutput.cpp
    audiodataoutput.cpp
    effect.cpp
    audiooutput.cpp
    mediaobject.cpp
    videodataoutput.cpp
    visualization.cpp
    backend.cpp
    volumefadereffect.cpp
    bytestream.cpp
    bytestreamplugin.cpp
    net_buf_ctrl.c
    volumefader_plugin.cpp
    mixer_plugin.cpp
    plugins.c
    videowidget.cpp
   )

macro_optional_find_package(XCB)
if(LIBXCB_FOUND AND XINE_XCB_FOUND)
  include_directories(${LIBXCB_INCLUDE_DIR})
  add_definitions(${LIBXCB_DEFINITIONS})
else(LIBXCB_FOUND AND XINE_XCB_FOUND)
  add_definitions(-DPHONON_XINE_NO_VIDEOWIDGET)
  message(STATUS "Cannot compile VideoWidget for the xine backend. Needs xine version 1.1.5 or later and libxcb.")
endif(LIBXCB_FOUND AND XINE_XCB_FOUND)

kde4_add_plugin(phonon_xine ${phonon_xine_PART_SRCS})
target_link_libraries(phonon_xine ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS} ${XINE_LIBRARY} ${KDE4_KAUDIODEVICELIST_LIBS})
if(LIBXCB_FOUND AND XINE_XCB_FOUND)
  target_link_libraries(phonon_xine ${LIBXCB_LIBRARIES})
endif(LIBXCB_FOUND AND XINE_XCB_FOUND)

install(TARGETS phonon_xine DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES xine.desktop DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)
kde4_install_icons(${ICON_INSTALL_DIR})
