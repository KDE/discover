# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

qt_add_dbus_interface(extra_SRCS org.freedesktop.login1.Manager.xml Login1ManagerInterface)
kcoreaddons_add_plugin(discover-notifier
    SOURCES
        module.cpp
        UnattendedUpdates.cpp
        ${extra_SRCS}
    INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(discover-notifier
    Qt6::Network
    KF6::DBusAddons
    KF6::IdleTime
    KF6::CoreAddons
    KF6::ConfigGui # KConfigSkeleton
)
kconfig_add_kcfg_files(discover-notifier ../kcm/updatessettings.kcfgc GENERATE_MOC)
