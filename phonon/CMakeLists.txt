# known limitations:
#  xinelib 1.1.1 adds audio
#  xinelib 1.1.2 adds audio with gapless playback
#  xinelib 1.1.5 + xcb adds video
#  xinelib 1.1.7 does not abort() when unplugging a USB device

# If you want to build KDE without any multimedia support
# define the cmake variable KDE4_DISABLE_MULTIMEDIA, i.e,
# % cmake -DKDE4_DISABLE_MULTIMEDIA=TRUE
#

if(KDE4_DISABLE_MULTIMEDIA)
  message(STATUS "NOTICE: Multimedia support DISABLED (KDE4_DISABLE_MULTIMEDIA!=0)")
else(KDE4_DISABLE_MULTIMEDIA)

  macro_optional_find_package(Xine)
  macro_log_feature(XINE_FOUND "Xine" "xine-lib is required for the multimedia backend" "http://xinehq.de/index.php/releases" TRUE "1.1.1" "The xine development libraries (v1.1.1 or above) are required to provide audio and video support. Run cmake again with the variable KDE4_DISABLE_MULTIMEDIA set if you really want to build KDE without multimedia support." )
  if(XINE_FOUND)
    set(XINE_BUGFIX_VERSION 7)
    # provide extra nice messages depending on the XINE_BUGFIX_VERSION
    if(XINE_BUGFIX_VERSION LESS 7)
      if(XINE_BUGFIX_VERSION LESS 5)
        if(XINE_BUGFIX_VERSION LESS 2)
	  macro_log_feature(TRUE "Xine" "xine-lib v1.1.${XINE_BUGFIX_VERSION} was found on your system. This version does not provide video support and is known to have problems with audio playback. Recommend upgrading to version 1.1.5 or above." "http://xinehq.de/index.php/releases")
        else(XINE_BUGFIX_VERSION LESS 2)
	  macro_log_feature(TRUE "Xine" "xine-lib v1.1.${XINE_BUGFIX_VERSION} was found on your system. This version does not provide video support. Recommend upgrading to version 1.1.5 or above." "http://xinehq.de/index.php/releases")
        endif(XINE_BUGFIX_VERSION LESS 2)
      else(XINE_BUGFIX_VERSION LESS 5)
	macro_log_feature(TRUE "Xine" "xine-lib v1.1.${XINE_BUGFIX_VERSION} was found on your system. This version provides audio and video support but is known to have problems with USB devices. Consider upgrading to version 1.1.7 or above." "http://xinehq.de/index.php/releases")
      endif(XINE_BUGFIX_VERSION LESS 5)
    endif(XINE_BUGFIX_VERSION LESS 7)
    macro_optional_add_subdirectory(xine)
  endif(XINE_FOUND)

endif(KDE4_DISABLE_MULTIMEDIA)
