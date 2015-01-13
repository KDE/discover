set(pkbackend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
)

add_library(muon-pkbackend MODULE ${pkbackend_SRCS})
target_link_libraries(muon-pkbackend PRIVATE MuonCommon Qt5::Core PK::packagekitqt5 KF5::ConfigGui KF5::Service AppstreamQt)

install(TARGETS muon-pkbackend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-packagekit-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

#notifier
add_library(MuonPackageKitNotifier MODULE PackageKitNotifier.cpp)
target_link_libraries(MuonPackageKitNotifier PRIVATE PK::packagekitqt5 MuonNotifiers)

install(TARGETS MuonPackageKitNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/muon-notifier)
