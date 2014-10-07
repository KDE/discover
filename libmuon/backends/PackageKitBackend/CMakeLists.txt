set(pkbackend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
)

add_library(muon-pkbackend MODULE ${pkbackend_SRCS})
target_link_libraries(muon-pkbackend PRIVATE muonprivate Qt5::Core PK::packagekitqt5 KF5::ConfigGui KF5::Service AppstreamQt)

install(TARGETS muon-pkbackend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-packagekit-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

#notifier
add_library(kded_muonpackagekitnotifier MODULE PackageKitNotifier.cpp)
target_link_libraries(kded_muonpackagekitnotifier PRIVATE PK::packagekitqt5 KF5::Service KF5::ConfigGui muonprivate)

install(TARGETS kded_muonpackagekitnotifier DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muonpackagekitnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
