set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

set_source_files_properties(org.projectatomic.rpmostree1.xml PROPERTIES
   INCLUDE dbustypes.h
)
set(rpmostree1_xml ${CMAKE_INSTALL_PREFIX}/${DBUS_INTERFACES_INSTALL_DIR}/org.projectatomic.rpmostree1.xml)
qt_add_dbus_interface(ostree1_SRCS org.projectatomic.rpmostree1.xml rpmostree1)

add_library(rpm-ostree-backend MODULE RpmOstreeResource.cpp RpmOstreeBackend.cpp RpmOstreeTransaction.cpp RpmOstree.qrc ${ostree1_SRCS})
target_link_libraries(rpm-ostree-backend PRIVATE Discover::Common Qt5::DBus KF5::CoreAddons KF5::I18n)

install(TARGETS rpm-ostree-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)

add_library(rpm-ostree-notifier MODULE RpmOstreeNotifier.cpp)
target_link_libraries(rpm-ostree-notifier Discover::Notifiers)
set_target_properties(rpm-ostree-notifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS rpm-ostree-notifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
