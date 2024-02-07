set(steamos-backend_SRCS
    SteamOSResource.h
    SteamOSResource.cpp
    SteamOSBackend.h
    SteamOSBackend.cpp
    SteamOSTransaction.h
    SteamOSTransaction.cpp
    dbushelpers.h
    dbushelpers.cpp
)

set(atomupd1_xml com.steampowered.Atomupd1.xml)
set_source_files_properties(${atomupd1_xml} PROPERTIES
    INCLUDE "dbushelpers.h"
    NA_NAMESPACE TRUE
)
qt_add_dbus_interface(steamos-backend_SRCS ${atomupd1_xml} atomupd1)
qt_add_dbus_interface(steamos-backend_SRCS org.freedesktop.DBus.Properties.xml dbusproperties_interface)

ecm_qt_declare_logging_category(steamos-backend_SRCS
    HEADER libdiscover_steamos_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_STEAMOS_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.steamos
    DESCRIPTION "libdiscover (backend - SteamOS)"
    EXPORT DISCOVER
)

add_library(steamos-backend MODULE ${steamos-backend_SRCS})
target_link_libraries(steamos-backend
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::DBus
        KF6::CoreAddons
        KF6::ConfigCore
        Discover::Common
        QCoro::Core
        QCoro::DBus
)

install(TARGETS steamos-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
