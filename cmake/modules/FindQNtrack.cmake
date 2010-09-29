# - Try to find the QNtrack library
# Once done this will define
#
#  QNTRACK_FOUND - system has the CK Connector
#  QNTRACK_INCLUDE_DIR - the CK Connector include directory
#  QNTRACK_LIBRARIES - the libraries needed to use CK Connector

# Copyright (C) 2010 Sune Vuorela <sune@debian.org>
# modeled after FindCkConnector.cmake:
# Copyright (c) 2008, Kevin Kofler, <kevin.kofler@chello.at>
# modeled after FindLibArt.cmake:
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if(QNTRACK_INCLUDE_DIR AND QNTRACK_LIBRARIES)

  # in cache already
  SET(QNTRACK_FOUND TRUE)

else (QNTRACK_INCLUDE_DIR AND QNTRACK_LIBRARIES)

  IF (NOT WIN32)
    FIND_PACKAGE(PkgConfig)
    IF (PKG_CONFIG_FOUND)
      # use pkg-config to get the directories and then use these values
      # in the FIND_PATH() and FIND_LIBRARY() calls
      pkg_check_modules(_QNTRACK_PC QUIET libntrack-qt4 )
    ENDIF (PKG_CONFIG_FOUND)
  ENDIF (NOT WIN32)

  FIND_PATH(QNTRACK_QT_INCLUDE_DIR QNtrack.h
     ${_QNTRACK_PC_INCLUDE_DIRS}
  )
  #Hide from cmake user interfaces
  SET(QNTRACK_QT_INCLUDE_DIR ${QNTRACK_QT_INCLUDE_DIR} CACHE INTERNAL "" FORCE)

  FIND_PATH(NTRACK_INCLUDE_DIR ntrackmonitor.h
     ${_QNTRACK_PC_INCLUDE_DIRS}
  )
  #Hide from cmake user interfaces
  SET(NTRACK_INCLUDE_DIR ${NTRACK_INCLUDE_DIR} CACHE INTERNAL "" FORCE)

  FIND_LIBRARY(QNTRACK_LIBRARIES NAMES ntrack-qt4
     PATHS
     ${_QNTRACK_PC_LIBDIR}
  )


  if (QNTRACK_QT_INCLUDE_DIR AND NTRACK_INCLUDE_DIR AND QNTRACK_LIBRARIES)
     set(QNTRACK_FOUND TRUE)
     set(QNTRACK_INCLUDE_DIR ${QNTRACK_QT_INCLUDE_DIR} ${NTRACK_INCLUDE_DIR})
  endif (QNTRACK_QT_INCLUDE_DIR AND NTRACK_INCLUDE_DIR AND QNTRACK_LIBRARIES)


  if (QNTRACK_FOUND)
     if (NOT QNtrack_FIND_QUIETLY)
        message(STATUS "Found QNtrack: ${QNTRACK_LIBRARIES}")
     endif (NOT QNtrack_FIND_QUIETLY)
  else (QNTRACK_FOUND)
     if (QNtrack_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find QNtrack")
     endif (QNtrack_FIND_REQUIRED)
  endif (QNTRACK_FOUND)

  MARK_AS_ADVANCED(QNTRACK_INCLUDE_DIR QNTRACK_LIBRARIES)

endif (QNTRACK_INCLUDE_DIR AND QNTRACK_LIBRARIES)
