# known limitations:
#  xinelib 1.1.1 adds audio
#  xinelib 1.1.2 adds audio with gapless playback
#  xinelib 1.1.5 + xcb adds video
#  xinelib 1.1.7 does not abort() when unplugging a USB device

# If you want to build KDE without any multimedia support
# define the cmake variable KDE4_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDE4_DISABLE_MULTIMEDIA=ON
#

OPTION(KDE4_DISABLE_MULTIMEDIA "Disable multimedia support (default: off)" OFF)

if(KDE4_DISABLE_MULTIMEDIA)
   message(STATUS "NOTICE: Multimedia support DISABLED (KDE4_DISABLE_MULTIMEDIA == ON)")
else(KDE4_DISABLE_MULTIMEDIA)
   macro_optional_find_package(Xine)

   # if no xinelib at all is available say that we want 1.1.7
   macro_log_feature(XINE_FOUND "Xine" "xine-lib is required for the multimedia backend" "http://sourceforge.net/project/showfiles.php?group_id=9655&package_id=9732" TRUE "1.1.7" "The xine development libraries (v1.1.7 or above) are required to provide audio and video support. Using the latest version is recommended. Run cmake again with the variable KDE4_DISABLE_MULTIMEDIA set if you really want to build KDE without multimedia support." )
   if(XINE_FOUND)
      string(REGEX REPLACE "^[0-9]*\\.([0-9]*)\\.[0-9]*$" "\\1" XINE_MINOR_VERSION ${XINE_VERSION})
      string(REGEX REPLACE "^[0-9]*\\.[0-9]*\\." "" XINE_BUGFIX_VERSION ${XINE_VERSION})
      # if an older xinelib is installed give a warning about problems but continue
      if(XINE_MINOR_VERSION LESS 2 AND XINE_BUGFIX_VERSION LESS 7)
         if(XINE_BUGFIX_VERSION LESS 5)
            macro_log_feature(TRUE "Xine" "xine-lib v${XINE_VERSION} was found on your system. This version does not provide video support and has known security issues. Recommend upgrading to version 1.1.7 or above." "http://sourceforge.net/project/showfiles.php?group_id=9655&package_id=9732")
         else(XINE_BUGFIX_VERSION LESS 5)
            macro_log_feature(TRUE "Xine" "xine-lib v${XINE_VERSION} was found on your system. This version is known to have problems when a device is unplugged. Consider upgrading to version 1.1.7 or above." "http://sourceforge.net/project/showfiles.php?group_id=9655&package_id=9732")
         endif(XINE_BUGFIX_VERSION LESS 5)
      endif(XINE_MINOR_VERSION LESS 2 AND XINE_BUGFIX_VERSION LESS 7)
      macro_optional_add_subdirectory(xine)
   endif(XINE_FOUND)
endif(KDE4_DISABLE_MULTIMEDIA)
