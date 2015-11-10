set(pkbackend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
)

add_library(packagekit-backend MODULE ${pkbackend_SRCS})
target_link_libraries(packagekit-backend PRIVATE Discover::Common Qt5::Core PK::packagekitqt5 KF5::ConfigGui KF5::Service AppstreamQt)

install(TARGETS packagekit-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES packagekit-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
install(FILES packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

#notifier
add_library(MuonPackageKitNotifier MODULE PackageKitNotifier.cpp)
target_link_libraries(MuonPackageKitNotifier PRIVATE PK::packagekitqt5 Discover::Notifiers)

install(TARGETS MuonPackageKitNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
