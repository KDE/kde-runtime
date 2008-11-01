alsa_version_string(ALSA_VERSION_STR)
set(HAVE_CURRENT_ALSA FALSE)
if(ALSA_VERSION_STR AND NOT WIN32)
   if(ALSA_VERSION_STR MATCHES "^1\\.(0\\.(1[4-9]|[2-9][0-9]+)|[1-9][0-9]*\\.)")
      set(HAVE_CURRENT_ALSA TRUE)
   endif(ALSA_VERSION_STR MATCHES "^1\\.(0\\.(1[4-9]|[2-9][0-9]+)|[1-9][0-9]*\\.)")
endif(ALSA_VERSION_STR AND NOT WIN32)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   macro_log_feature(HAVE_CURRENT_ALSA "ALSA" "current alsa-lib is needed for dmix and virtual device listing" "http://www.alsa-project.org/" TRUE "1.0.14a")
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

macro_optional_find_package(PulseAudio)
macro_log_feature(PULSEAUDIO_FOUND "PulseAudio" "libpulse is needed to allow audio playback via the PulseAudio soundserver when it is running" "http://www.pulseaudio.org/" FALSE)

if(PULSEAUDIO_FOUND)
   add_definitions(-DHAVE_PULSEAUDIO)
else(PULSEAUDIO_FOUND)
   set(PULSEAUDIO_INCLUDE_DIR "")
   set(PULSEAUDIO_LIBRARY "")
endif(PULSEAUDIO_FOUND)

include_directories(${ALSA_INCLUDES} ${PULSEAUDIO_INCLUDE_DIR})

set(kded_phonon_SRCS
   phononserver.cpp
   audiodevice.cpp
   audiodeviceaccess.cpp
   hardwaredatabase.cpp
   )

kde4_add_plugin(kded_phononserver ${kded_phonon_SRCS})
target_link_libraries(kded_phononserver ${KDE4_KDEUI_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_SOLID_LIBS} ${ASOUND_LIBRARY} ${PULSEAUDIO_LIBRARY})
install(TARGETS kded_phononserver  DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES phononserver.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

install(FILES hardwaredatabase DESTINATION ${DATA_INSTALL_DIR}/libphonon)
