# If you want to build kdebase without any multimedia support
# define the cmake variable KDEBASE_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDEBASE_DISABLE_MULTIMEDIA=ON
#

option(KDEBASE_DISABLE_MULTIMEDIA "Disable multimedia support (default: off)" OFF)

# print a message for backward compatibility:
if(KDE4_DISABLE_MULTIMEDIA)
   message(STATUS "*********** The switch KDE4_DISABLE_MULTIMEDIA is not supported anymore, use KDEBASE_DISABLE_MULTIMEDIA instead ************")
endif(KDE4_DISABLE_MULTIMEDIA)


if(KDEBASE_DISABLE_MULTIMEDIA)
   message(STATUS "NOTICE: Multimedia support DISABLED (KDEBASE_DISABLE_MULTIMEDIA == ON)")
else(KDEBASE_DISABLE_MULTIMEDIA)
   set(PHONON_MIN_VERSION "4.2.96")
   macro_ensure_version("${PHONON_MIN_VERSION}" "${PHONON_VERSION}" _phonon_version_ok)
   macro_log_feature(_phonon_version_ok "Phonon" "Phonon library" "svn.kde.org/home/kde/trunk/kdesupport/phonon" TRUE "${PHONON_MIN_VERSION}" "STRONGLY RECOMMENDED: Required for playing audio and video throughout KDE")

   find_package(Alsa)
   alsa_configure_file(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

   if(NOT ALSA_FOUND)
      set(ALSA_INCLUDES "")
      set(ASOUND_LIBRARY "")
   endif(NOT ALSA_FOUND)

   add_subdirectory(kded-module)
   add_subdirectory(platform_kde)
   add_subdirectory(tests)
   add_subdirectory(kcm)
endif(KDEBASE_DISABLE_MULTIMEDIA)
