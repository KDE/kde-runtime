# - Try to find libxcb
# Once done this will define
#
#  LIBXCB_FOUND - system has libxcb
#  LIBXCB_LIBRARIES - Link these to use libxcb
#  LIBXCB_INCLUDE_DIR - the libxcb include dir
#  LIBXCB_DEFINITIONS - compiler switches required for using libxcb

# Copyright (c) 2007, Matthias Kretz, <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


IF (NOT WIN32)
  IF (LIBXCB_INCLUDE_DIR AND LIBXCB_LIBRARIES)
    # in cache already
    SET(XCB_FIND_QUIETLY TRUE)
  ENDIF (LIBXCB_INCLUDE_DIR AND LIBXCB_LIBRARIES)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)
  PKGCONFIG(xcb _LibXCBIncDir _LibXCBLinkDir _LibXCBLinkFlags _LibXCBCflags)
	  
  SET(LIBXCB_DEFINITIONS ${_LibXCBCflags})

  FIND_PATH(LIBXCB_INCLUDE_DIR xcb/xcb.h
    ${_LibXCBIncDir}
    )

  FIND_LIBRARY(LIBXCB_LIBRARIES NAMES xcb libxcb
    PATHS
    ${_LibXCBLinkDir}
    )

  IF (LIBXCB_INCLUDE_DIR AND LIBXCB_LIBRARIES)
    SET(LIBXCB_FOUND TRUE)
  ELSE (LIBXCB_INCLUDE_DIR AND LIBXCB_LIBRARIES)
    SET(LIBXCB_FOUND FALSE)
  ENDIF (LIBXCB_INCLUDE_DIR AND LIBXCB_LIBRARIES)

  IF (LIBXCB_FOUND)
    IF (NOT XCB_FIND_QUIETLY)
      MESSAGE(STATUS "Found LibXCB: ${LIBXCB_LIBRARIES}")
    ENDIF (NOT XCB_FIND_QUIETLY)
  ELSE (LIBXCB_FOUND)
    IF (XCB_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could NOT find LibXCB")
    ENDIF (XCB_FIND_REQUIRED)
  ENDIF (LIBXCB_FOUND)

  MARK_AS_ADVANCED(LIBXCB_INCLUDE_DIR LIBXCB_LIBRARIES XCBPROC_EXECUTABLE)
ENDIF (NOT WIN32)
