#
# CMake module to locate the AkabeiClient library.
#
# Once done this will define:
#
#  AKABEICLIENT_FOUND — System has AkabeiClient library.
#  AKABEICLIENT_INCLUDE_DIR — AkabeiClient library include directory.
#  AKABEICLIENT_LIBRARIES — Paths to the library files of the AkabeiClient library.
#
# Copyright © 2007 Joris Guisson <joris.guisson@gmail.com> (This was based upon FindKTorrent.cmake)
# Copyright © 2007 Charles Connell <charles@connells.org> (This was based upon FindKopete.cmake)
# Copyright © 2010 Lukas Appelhans <l.appelhans@gmx.de>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Search for the AkabeiClient library include path if not provided.
if(NOT AKABEICLIENT_INCLUDE_DIR)

  FIND_PATH(AKABEICLIENT_INCLUDE_DIR 
    NAMES
    akabeiclientbackend.h
    PATHS
    ${INCLUDE_INSTALL_DIR}
    PATH_SUFFIXES
    akabeiclient
    )

endif(NOT AKABEICLIENT_INCLUDE_DIR)

# Search for the AkabeiClient library files.
FIND_LIBRARY(AKABEICLIENT_LIBRARIES 
  NAMES
  akabeiclient
  PATHS
  ${AKABEICLIENT_LIB_DIR}
  ${LIB_INSTALL_DIR}
)

if(AKABEICLIENT_INCLUDE_DIR AND AKABEICLIENT_LIBRARIES)

  # Read from cache.
  set(AKABEICLIENT_FOUND TRUE)

  # Announce.
  if(NOT AKABEICLIENT_FIND_QUIETLY)
    message(STATUS "Looking for AkabeiClient library… FOUND.")
  endif(NOT AKABEICLIENT_FIND_QUIETLY)

# Akabei could not be found.
else(AKABEICLIENT_INCLUDE_DIR AND AKABEICLIENT_LIBRARIES)

  message(STATUS "Looking for AkabeiClient library… NOT FOUND.")

  function(MSG_AKABEI_INCLUDES SEVERITY)
    message(${SEVERITY} "  - Could not find AkabeiClient library header files.")
    message(${SEVERITY} "    Use AKABEICLIENT_INCLUDE_DIR to point to the directory where they are located.")
  endfunction(MSG_AKABEI_INCLUDES)

  function(MSG_AKABEI_LIBRARY SEVERITY)
    message(${SEVERITY} "  - Could not find AkabeiClient library files.")
    message(${SEVERITY} "    Use AKABEICLIENT_LIB_DIR to point to the directory where they are located.")
  endfunction(MSG_AKABEI_LIBRARY)

  if(AKABEICLIENT_FIND_REQUIRED)
    if(NOT AKABEICLIENT_INCLUDE_DIR)
      MSG_AKABEI_INCLUDES("FATAL_ERROR")
    endif(NOT AKABEICLIENT_INCLUDE_DIR)
    if(NOT AKABEICLIENT_LIBRARIES)
      MSG_AKABEI_LIBRARY("FATAL_ERROR")
    endif(NOT AKABEICLIENT_LIBRARIES)
  else(AKABEICLIENT_FIND_REQUIRED)
    if(NOT AKABEICLIENT_INCLUDE_DIR)
      MSG_AKABEI_INCLUDES("STATUS")
    endif(NOT AKABEICLIENT_INCLUDE_DIR)
    if(NOT AKABEICLIENT_LIBRARIES)
      MSG_AKABEI_LIBRARY("STATUS")
    endif(NOT AKABEICLIENT_LIBRARIES)
  endif(AKABEICLIENT_FIND_REQUIRED)

endif(AKABEICLIENT_INCLUDE_DIR AND AKABEICLIENT_LIBRARIES)
