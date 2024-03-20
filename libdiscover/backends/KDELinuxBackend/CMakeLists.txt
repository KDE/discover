# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

add_library(kde-linux-backend MODULE
    KDELinuxBackend.cpp
    KDELinuxResource.cpp
    KDELinuxTransaction.cpp
)
target_link_libraries(kde-linux-backend
    Qt::Core
    Qt::DBus
    KF6::CoreAddons
    Discover::Common
)
set_target_properties(kde-linux-backend PROPERTIES OUTPUT_NAME "discover/kde-linux-backend")

ecm_qt_declare_logging_category(kde-linux-backend
    HEADER debug.h
    IDENTIFIER KDELINUX
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.kde.linux
    DESCRIPTION "libdiscover (backend - KDE Linux)"
    EXPORT DISCOVER
)

install(TARGETS kde-linux-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)

add_subdirectory(polkit)
