# - Try to find Upnp
# Once done this will define
#
#  UPNP_FOUND - system has Upnp
#  UPNP_INCLUDE_DIR - the Upnp include directory
#  UPNP_LIBRARIES - Link these to use Upnp
#  UPNP_DEFINITIONS - Compiler switches required for using Upnp
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( UPNP_INCLUDE_DIR AND UPNP_LIBRARIES )
   # in cache already
   SET(Upnp_FIND_QUIETLY TRUE)
endif ( UPNP_INCLUDE_DIR AND UPNP_LIBRARIES )

FIND_PATH(UPNP_INCLUDE_DIR NAMES upnp/device.h
)

FIND_LIBRARY(UPNP_LIBRARIES NAMES upnp
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Upnp DEFAULT_MSG UPNP_INCLUDE_DIR UPNP_LIBRARIES )

# show the UPNP_INCLUDE_DIR and UPNP_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(UPNP_INCLUDE_DIR UPNP_LIBRARIES )

