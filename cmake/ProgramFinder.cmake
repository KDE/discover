# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

# Finds a random program as cmake package. Useful to find runtime deps.
# Excess call arguments are passed to the internal find_program() call.
#
#  $package_FOUND - true if found
#  $package_PATH - path to the bin (only when found)
macro(program_finder program)
    find_program(${CMAKE_FIND_PACKAGE_NAME}_PATH ${program} ${ARGN})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
        FOUND_VAR ${CMAKE_FIND_PACKAGE_NAME}_FOUND
        REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_PATH
    )
    mark_as_advanced(${CMAKE_FIND_PACKAGE_NAME}_PATH)
endmacro()
