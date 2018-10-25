# CMake support for Fwupd library
# Once done this will define
#
#  LIBFWUPD_FOUND - system has the fwupd library
#  LIBFWUPD_INCLUDE_DIR - the Fwupd include directory
#  LIBFWUPD_LIBRARY - Link this to use the fwupd 
#
# License:
#
#   Copyright © 2018, Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use, copy,
#   modify, merge, publish, distribute, sublicense, and/or sell copies
#   of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.

if(LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES AND LIBFWUPD_CONFIG_INCLUDE_DIRS)
    set(LIBFWUPD_FOUND TRUE)

else ()
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBFWUPD_PKG fwupd>=${LIBFWUPD_FIND_VERSION})
    endif()

    find_library (LIBFWUPD_LIBRARIES
        NAMES fwupd libfwupd
    )
  
    find_path (LIBFWUPD_INCLUDE_DIRS
        NAMES fwupd.h
        HINTS ${LIBFWUPD_PKG_INCLUDE_DIRS}
        PATH_SUFFIXES fwupd-1
    )

    if(LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES AND PKG_CONFIG_FOUND)
        if(((LIBFWUPD_PKG_VERSION VERSION_GREATER LIBFWUPD_FIND_VERSION) OR (LIBFWUPD_PKG_VERSION VERSION_EQUAL LIBFWUPD_FIND_VERSION)))
            set(LIBFWUPD_FOUND TRUE)
        endif()
    endif()
endif()

if (LIBFWUPD_FOUND)
    add_library(LIBFWUPD SHARED IMPORTED)
    set_target_properties(LIBFWUPD PROPERTIES
       INTERFACE_INCLUDE_DIRECTORIES ${LIBFWUPD_INCLUDE_DIRS}
       INTERFACE_LINK_LIBRARIES "${LIBFWUPD_PKG_LINK_LIBRARIES}"
       IMPORTED_LOCATION ${LIBFWUPD_LIBRARIES}
       ) 
endif()
