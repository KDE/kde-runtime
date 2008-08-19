# If you want to build KDE without any multimedia support
# define the cmake variable KDE4_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDE4_DISABLE_MULTIMEDIA=ON
#

include(UsePkgConfig)
OPTION(KDE4_DISABLE_MULTIMEDIA "Disable multimedia support (default: off)" OFF)

if(KDE4_DISABLE_MULTIMEDIA)
   message(STATUS "NOTICE: Multimedia support DISABLED (KDE4_DISABLE_MULTIMEDIA == ON)")
else(KDE4_DISABLE_MULTIMEDIA)
   FIND_PACKAGE(Alsa)
   ALSA_CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

   if(NOT ALSA_FOUND)
      set(ALSA_INCLUDES "")
      set(ASOUND_LIBRARY "")
   endif(NOT ALSA_FOUND)
   set(KDE4_KAUDIODEVICELIST_LIBS kaudiodevicelist ${KDE4_SOLID_LIBS} ${KDE4_PHONON_LIBS} ${KDE4_KDECORE_LIBS} ${ASOUND_LIBRARY})

   add_subdirectory(libkaudiodevicelist)
   add_subdirectory(platform_kde)
   add_subdirectory(tests)
   add_subdirectory(kcm)

endif(KDE4_DISABLE_MULTIMEDIA)
