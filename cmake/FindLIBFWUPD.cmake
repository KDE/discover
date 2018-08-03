# - Try to find the Fwupd library
# Once done this will define
#
#  LIBFWUPD_FOUND - system has the fwupd library
#  LIBFWUPD_INCLUDE_DIR - the Fwupd include directory
#  LIBFWUPD_LIBRARY - Link this to use the fwupd 
#
# Copyright © 2018, Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

if(LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES)
    set(LIBFWUPD_FOUND TRUE)

else (LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES)

    find_library (LIBFWUPD_LIBRARIES
        NAMES  fwupd libfwupd
    )
  
    find_path (LIBFWUPD_INCLUDE_DIRS
        NAMES fwupd.h
        PATH_SUFFIXES fwupd-1
        HINTS fwupd-1/libfwupd
    )
    
    if(LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES)
        set(LIBFWUPD_FOUND TRUE)

endif (LIBFWUPD_INCLUDE_DIRS AND LIBFWUPD_LIBRARIES)

if (LIBFWUPD_FOUND)
    add_library(LIBFWUPD SHARED IMPORTED)
    set_target_properties(LIBFWUPD PROPERTIES
       INTERFACE_INCLUDE_DIRECTORIES ${LIBFWUPD_INCLUDE_DIRS}
       IMPORTED_LOCATION ${LIBFWUPD_LIBRARIES}
       ) 
endif (LIBFWUPD_FOUND)
