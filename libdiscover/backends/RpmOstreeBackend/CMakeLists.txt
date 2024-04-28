set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

find_file(RpmOstreeDBusInterface org.projectatomic.rpmostree1.xml /usr/share/dbus-1/interfaces/)
qt_add_dbus_interface(RpmOstreeDBusInterface_SRCS ${RpmOstreeDBusInterface} RpmOstreeDBusInterface)

set(rpm-ostree-backend_SRCS
    OstreeFormat.cpp
    RpmOstreeResource.cpp
    RpmOstreeBackend.cpp
    RpmOstreeSourcesBackend.cpp
    RpmOstreeTransaction.cpp
    ${RpmOstreeDBusInterface_SRCS}
)
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
)
set_target_properties(rpm-ostree-notifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
