set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

set_source_files_properties(org.projectatomic.rpmostree1.xml PROPERTIES
   INCLUDE dbustypes.h
)
set(rpmostree1_xml ${CMAKE_INSTALL_PREFIX}/${DBUS_INTERFACES_INSTALL_DIR}/org.projectatomic.rpmostree1.xml)
qt5_add_dbus_interface(ostree1_SRCS org.projectatomic.rpmostree1.xml rpmostree1)

add_library(rpmostree-backend MODULE RpmOstreeResource.cpp RpmOstreeBackend.cpp RpmOstreeTransaction.cpp RpmOstree.qrc ${ostree1_SRCS})
target_link_libraries(rpmostree-backend PRIVATE Discover::Common Qt5::DBus KF5::CoreAddons KF5::I18n)

install(TARGETS rpmostree-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)

add_library(RpmOstreeNotifier MODULE RpmOstreeNotifier.cpp)
target_link_libraries(RpmOstreeNotifier Discover::Notifiers)
set_target_properties(RpmOstreeNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS RpmOstreeNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
