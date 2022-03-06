set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

# TODO: Once https://github.com/coreos/rpm-ostree/pull/3112 is included in an
# rpm-ostree release, use the two following lines and drop the local DBus XML
# interface local copy (org.projectatomic.rpmostree1.xml).
# find_file(RpmOstreeDBusInterface org.projectatomic.rpmostree1.xml /usr/share/dbus-1/interfaces/)
# qt_add_dbus_interface(RpmOstreeDBusInterface_SRCS ${RpmOstreeDBusInterface} RpmOstreeDBusInterface)
set(RpmOstreeDBusInterface ${CMAKE_INSTALL_PREFIX}/${DBUS_INTERFACES_INSTALL_DIR}/org.projectatomic.rpmostree1.xml)
qt_add_dbus_interface(RpmOstreeDBusInterface_SRCS org.projectatomic.rpmostree1.xml RpmOstreeDBusInterface)

add_library(rpm-ostree-backend MODULE RpmOstreeResource.cpp RpmOstreeBackend.cpp RpmOstreeSourcesBackend.cpp RpmOstreeTransaction.cpp RpmOstree.qrc ${RpmOstreeDBusInterface_SRCS})
target_link_libraries(rpm-ostree-backend PRIVATE Discover::Common Qt5::DBus KF5::CoreAddons KF5::I18n PkgConfig::Ostree)

install(TARGETS rpm-ostree-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
install(FILES rpm-ostree-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

add_library(rpm-ostree-notifier MODULE RpmOstreeNotifier.cpp)
target_link_libraries(rpm-ostree-notifier Discover::Notifiers PkgConfig::Ostree)
set_target_properties(rpm-ostree-notifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS rpm-ostree-notifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)
