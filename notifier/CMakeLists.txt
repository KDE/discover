add_library(kded_MuonNotifier MODULE BackendNotifierFactory.cpp MuonNotifierModule.cpp)
target_link_libraries(kded_MuonNotifier
    KF5::DBusAddons
    KF5::Notifications
    KF5::ConfigCore
    KF5::I18n
    KF5::Service
    KF5::KIOWidgets
    KF5::IconThemes
    Qt5::DBus

    MuonNotifiers
)
install(TARGETS kded_MuonNotifier DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES MuonNotifier.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
