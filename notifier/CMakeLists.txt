add_definitions(-DTRANSLATION_DOMAIN=\"muon-notifier\")

add_library(MuonNotifierDeclarativePlugin BackendNotifierFactory.cpp MuonNotifier.cpp MuonNotifierDeclarativePlugin.cpp)
target_link_libraries(MuonNotifierDeclarativePlugin
    KF5::Notifications
    KF5::ConfigCore
    KF5::I18n
    KF5::Service
    KF5::KIOWidgets
    KF5::IconThemes
    Qt5::Qml

    MuonNotifiers
)
install(TARGETS MuonNotifierDeclarativePlugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/muonnotifier)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/muonnotifier)

find_package(KF5Plasma)
if(KF5Plasma_FOUND)
    plasma_install_package(plasmoid org.kde.muonnotifier)
endif()
