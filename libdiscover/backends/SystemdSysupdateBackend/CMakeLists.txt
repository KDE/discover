# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Lasath Fernando <devel@lasath.org>

set(systemdsysupdate-backend_SRCS
    SystemdSysupdateBackend.cpp
    SystemdSysupdateBackend.h
    SystemdSysupdateResource.cpp
    SystemdSysupdateResource.h
    SysupdateInternal.cpp
    SysupdateInternal.h
    SystemdSysupdateTransaction.cpp
    SystemdSysupdateTransaction.h
)

set(sysupdate1_xml org.freedesktop.sysupdate1.xml)
set_source_files_properties(${sysupdate1_xml} PROPERTIES
   INCLUDE "SysupdateInternal.h"
   NA_NAMESPACE TRUE
)
qt_add_dbus_interface(systemdsysupdate-backend_SRCS ${sysupdate1_xml} sysupdate1)

ecm_qt_declare_logging_category(systemdsysupdate-backend_SRCS
    HEADER libdiscover_systemdsysupdate_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.systemdsysupdate
    DESCRIPTION "libdiscover (backend - systemdsysupdate)"
    EXPORT DISCOVER
)

kcoreaddons_add_plugin(systemdsysupdate-backend SOURCES ${systemdsysupdate-backend_SRCS} INSTALL_NAMESPACE "discover")

target_link_libraries(systemdsysupdate-backend
    Qt::Core
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
    AppStreamQt
    QCoro::Core
    QCoro::DBus
    QCoro::Network
)
