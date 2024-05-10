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

kcoreaddons_add_plugin(steamos-backend SOURCES ${steamos-backend_SRCS} INSTALL_NAMESPACE "discover")

add_library(libdiscover-backend-steamos-logging-category STATIC
    ${libdiscover-backend-steamos-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-steamos-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-steamos-logging-category
    PRIVATE
        Qt::Core
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

target_link_libraries(steamos-backend
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)
