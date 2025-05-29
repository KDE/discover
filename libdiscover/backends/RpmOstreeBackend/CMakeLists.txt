ecm_qt_declare_logging_category(libdiscover-backend-rpm-ostree-logging-category_SRCS
    HEADER libdiscover_rpm-ostree_debug.h
    IDENTIFIER RPMOSTREE_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.rpm-ostree
    DESCRIPTION "libdiscover (backend - rpm-ostree)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-rpm-ostree-logging-category STATIC
    ${libdiscover-backend-rpm-ostree-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-rpm-ostree-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-rpm-ostree-logging-category
    PRIVATE
        Qt::Core
)

set(rpm-ostree-backend_SRCS
    OstreeFormat.cpp
    RpmOstreeResource.cpp
    RpmOstreeBackend.cpp
    RpmOstreeSourcesBackend.cpp
    RpmOstreeTransaction.cpp
)

if(RpmOstree_FOUND)
    find_file(RpmOstreeDBusInterface org.projectatomic.rpmostree1.xml /usr/share/dbus-1/interfaces/)
else()
    set(RpmOstreeDBusInterface org.projectatomic.rpmostree1.xml)
endif()
qt_add_dbus_interface(rpm-ostree-backend_SRCS ${RpmOstreeDBusInterface} RpmOstreeDBusInterface)

kcoreaddons_add_plugin(rpm-ostree-backend SOURCES ${rpm-ostree-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(rpm-ostree-backend
    PRIVATE
        Discover::Common
        Qt::DBus
        KF6::ConfigCore
        KF6::CoreAddons
        KF6::I18n
        PkgConfig::Ostree
        AppStreamQt
        libdiscover-backend-rpm-ostree-logging-category
)

install(FILES rpm-ostree-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

set(rpm-ostree-notifier_SRCS
    OstreeFormat.cpp
    RpmOstreeNotifier.cpp
)
kcoreaddons_add_plugin(rpm-ostree-notifier SOURCES ${rpm-ostree-notifier_SRCS} INSTALL_NAMESPACE "discover-notifier")
target_link_libraries(rpm-ostree-notifier
    Discover::Notifiers
    PkgConfig::Ostree
    libdiscover-backend-rpm-ostree-logging-category
)
set_target_properties(rpm-ostree-notifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
