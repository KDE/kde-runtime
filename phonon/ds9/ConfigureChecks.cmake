# Copyright (C) 2007 Trolltech ASA. All rights reserved.
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2 or 3 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

# We must find:
#      $DXSDK_DIR/include/d3d9.h
#      $DXDSK_DIR/$LIB/dxguid.lib
#      vmr9.h
#      dshow.h
#      strmiids.lib
#      dmoguids.lib
#      msdmo.lib
include(CheckCXXSourceCompiles)

macro_push_required_vars()

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} $ENV{DXSDK_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} dxguid strmiids dmoguids msdmo)

CHECK_CXX_SOURCE_COMPILES(
"#include <d3d9.h>
#include <dshow.h>
#include <vmr9.h>

int main() { }" BUILD_PHONON_DS9)

macro_pop_required_vars()

if (BUILD_PHONON_DS9)
   message(STATUS "Found DirectShow 9 support: $ENV{DXSDK_DIR}")
else (BUILD_PHONON_DS9)
   message(STATUS "DirectShow 9 support NOT found")
endif (BUILD_PHONON_DS9)
