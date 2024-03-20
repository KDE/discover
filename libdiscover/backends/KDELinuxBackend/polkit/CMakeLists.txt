# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

find_package(PolkitQt6-1)
set_package_properties(PolkitQt6-1 PROPERTIES TYPE REQUIRED PURPOSE "Doing systemd-sysupdate updates")

find_package(systemd-sysupdate)
set_package_properties(systemd-sysupdate PROPERTIES TYPE RUNTIME)

add_executable(plasma-discover-kde-linux-polkit-helper main.cpp)
target_link_libraries(plasma-discover-kde-linux-polkit-helper Qt6::DBus PolkitQt6-1::Core)
install(TARGETS plasma-discover-kde-linux-polkit-helper DESTINATION ${KDE_INSTALL_LIBEXECDIR_KF})

install(FILES org.kde.discover.kde.linux.conf DESTINATION ${KDE_INSTALL_DBUSDIR}/system.d)
install(FILES org.kde.discover.kde.linux.policy DESTINATION ${KDE_INSTALL_DATADIR}/polkit-1/actions)
configure_file(org.kde.discover.kde.linux.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.kde.linux.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.kde.linux.service DESTINATION ${KDE_INSTALL_DBUSSYSTEMSERVICEDIR})
