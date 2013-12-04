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
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

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
