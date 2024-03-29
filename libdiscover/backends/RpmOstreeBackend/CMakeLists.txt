set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

find_file(RpmOstreeDBusInterface org.projectatomic.rpmostree1.xml /usr/share/dbus-1/interfaces/)
qt_add_dbus_interface(RpmOstreeDBusInterface_SRCS ${RpmOstreeDBusInterface} RpmOstreeDBusInterface)

add_library(rpm-ostree-backend MODULE
    OstreeFormat.cpp
    RpmOstreeResource.cpp
    RpmOstreeBackend.cpp
    RpmOstreeSourcesBackend.cpp
    RpmOstreeTransaction.cpp
    ${RpmOstreeDBusInterface_SRCS}
)
target_link_libraries(rpm-ostree-backend
    PRIVATE
        Discover::Common
        Qt::DBus
        KF6::CoreAddons
        KF6::I18n
        PkgConfig::Ostree
        AppStreamQt
)

install(TARGETS rpm-ostree-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
install(FILES rpm-ostree-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

add_library(rpm-ostree-notifier MODULE
    OstreeFormat.cpp
    RpmOstreeNotifier.cpp
)
target_link_libraries(rpm-ostree-notifier
    Discover::Notifiers
    PkgConfig::Ostree
)
set_target_properties(rpm-ostree-notifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS rpm-ostree-notifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)
