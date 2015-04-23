# - Try to find the PackageKitQt2 library
# Once done this will define
#
#  PACKAGEKITQT2_FOUND - system has the PackageKitQt2 library
#  PACKAGEKITQT2_INCLUDEDIR - the PackageKitQt2 include directory
#  PACKAGEKITQT2_LIBRARY - Link this to use the PackageKitQt2
#
# Copyright © 2010, Mehrdad Momeny <mehrdad.momeny@gmail.com>
# Copyright © 2010, Harald Sitter <apachelogger@ubuntu.com>
# Copyright © 2013, Lukas Appelhans <l.appelhans@gmx.de>
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

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(QPACKAGEKIT2 packagekit-qt2>=0.8)
endif()

set(PackageKitQt2_FOUND FALSE)
if(QPACKAGEKIT2_FOUND)
    find_library(PACKAGEKITQT2_LIBRARY NAMES packagekit-qt2
        HINTS ${QPACKAGEKIT2_LIBRARIES}
    )

    find_path(PACKAGEKITQT2_INCLUDEDIR PackageKit/packagekit-qt2/daemon.h
        HINTS ${QPACKAGEKIT2_INCLUDEDIR}
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(PackageKitQt2 DEFAULT_MSG PACKAGEKITQT2_LIBRARY PACKAGEKITQT2_INCLUDEDIR)
    if(PACKAGEKITQT2_LIBRARY AND PACKAGEKITQT2_INCLUDEDIR)
        mark_as_advanced(PACKAGEKITQT2_INCLUDEDIR PACKAGEKITQT2_LIBRARY)
        set(PackageKitQt2_FOUND TRUE)
    endif()
endif()
