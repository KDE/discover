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

add_library(steamos-backend MODULE ${steamos-backend_SRCS})
target_link_libraries(steamos-backend Qt::Core Qt::Widgets Qt::DBus KF5::CoreAddons KF5::ConfigCore Discover::Common)

install(TARGETS steamos-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
