# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

find_package(SeleniumWebDriverATSPI)
set_package_properties(SeleniumWebDriverATSPI PROPERTIES
    DESCRIPTION "Server component for selenium tests using Linux accessibility infrastructure"
    PURPOSE "Needed for GUI tests"
    URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
    TYPE OPTIONAL
)
if(NOT SeleniumWebDriverATSPI_FOUND AND NOT "$ENV{KDECI_BUILD}" STREQUAL "TRUE") # always run on CI
    return()
endif()

cmake_host_system_information(RESULT DISTRO_ID QUERY DISTRIB_ID)
cmake_host_system_information(RESULT DISTRO_ID_LIKE QUERY DISTRIB_ID_LIKE)
list(PREPEND DISTRO_ID_LIKE ${DISTRO_ID})
string(FIND "${DISTRO_ID_LIKE}" "alpine" FINDINDEX)
if(NOT (FINDINDEX EQUAL -1))
    message(WARNING "Skipping appium tests on alpine")
    return()
endif()

add_test(
    NAME appium-flatpak
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/flatpak.py
)
set_tests_properties(appium-flatpak PROPERTIES TIMEOUT 600)
