# If you want to build kdebase without any multimedia support
# define the cmake variable KDEBASE_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDEBASE_DISABLE_MULTIMEDIA=ON
#

project(PHONON-KDE-RUNTIME)
find_package(KDE4 REQUIRED)

include(KDE4Defaults)
include(MacroLibrary)
include(CheckFunctionExists)
include(CheckIncludeFiles)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

option(KDEBASE_DISABLE_MULTIMEDIA "Disable multimedia support (default: off)" OFF)

# print a message for backward compatibility:
if(KDE4_DISABLE_MULTIMEDIA)
   message(STATUS "*********** The switch KDE4_DISABLE_MULTIMEDIA is not supported anymore, use KDEBASE_DISABLE_MULTIMEDIA instead ************")
endif(KDE4_DISABLE_MULTIMEDIA)


if(KDEBASE_DISABLE_MULTIMEDIA)
   message(STATUS "NOTICE: Multimedia support DISABLED (KDEBASE_DISABLE_MULTIMEDIA == ON)")
else(KDEBASE_DISABLE_MULTIMEDIA)
   set(PHONON_MIN_VERSION "4.4.57")
   macro_ensure_version("${PHONON_MIN_VERSION}" "${PHONON_VERSION}" _phonon_version_ok)
   macro_log_feature(_phonon_version_ok "Phonon" "Phonon library" "git://anongit.kde.org/phonon.git" TRUE "${PHONON_MIN_VERSION}" "STRONGLY RECOMMENDED: Required for playing audio and video throughout KDE")

   find_package(Alsa)
   option(WITH_ALSA "Use ALSA, if found" ON)
   if(NOT WITH_ALSA)
      set(ALSA_FOUND FALSE)
      set(ASOUND_LIBRARY NOTFOUND)
      set(HAVE_LIBASOUND2 FALSE)
      set(ALSA_INCLUDES NOTFOUND)
   endif(NOT WITH_ALSA)

   alsa_configure_file(${CMAKE_CURRENT_BINARY_DIR}/config-alsa.h)

   set(HAVE_CURRENT_ALSA FALSE)
   if(ALSA_FOUND)
      include_directories(${ALSA_INCLUDES})
      alsa_version_string(ALSA_VERSION_STR)
      if(ALSA_VERSION_STR AND NOT WIN32)
         if(ALSA_VERSION_STR MATCHES "^1\\.(0\\.(1[4-9]|[2-9][0-9]+)|[1-9][0-9]*\\.)")
            add_definitions(-DHAS_LIBASOUND_DEVICE_NAME_HINT)
            set(HAVE_CURRENT_ALSA TRUE)
         endif(ALSA_VERSION_STR MATCHES "^1\\.(0\\.(1[4-9]|[2-9][0-9]+)|[1-9][0-9]*\\.)")
      endif(ALSA_VERSION_STR AND NOT WIN32)
   endif(ALSA_FOUND)
   if(NOT WIN32)
      macro_log_feature(HAVE_CURRENT_ALSA "ALSA" "The Advanced Linux Sound Architecture (ALSA) library" "http://www.alsa-project.org/" FALSE "1.0.14a" "A current ALSA library is needed for dmix and virtual device listing")
   endif(NOT WIN32)

   add_subdirectory(kded-module)
   add_subdirectory(platform_kde)
   macro_optional_add_subdirectory(tests)
   macro_optional_add_subdirectory(kcm)
   #macro_optional_add_subdirectory(libkaudiodevicelist)
endif(KDEBASE_DISABLE_MULTIMEDIA)
