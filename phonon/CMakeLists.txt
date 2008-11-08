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
   IF(ALSA_FOUND)
   ALSA_CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

   if(NOT ALSA_FOUND)
      set(ALSA_INCLUDES "")
      set(ASOUND_LIBRARY "")
   endif(NOT ALSA_FOUND)

   add_subdirectory(kded-module)
   add_subdirectory(platform_kde)
   add_subdirectory(tests)
   add_subdirectory(kcm)

    ENDIF(ALSA_FOUND)
endif(KDE4_DISABLE_MULTIMEDIA)
