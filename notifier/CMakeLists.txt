add_definitions(-DTRANSLATION_DOMAIN=\"plasma-discover-notifier\")

add_library(DiscoverNotifierDeclarativePlugin BackendNotifierFactory.cpp DiscoverNotifier.cpp DiscoverNotifierDeclarativePlugin.cpp)
target_link_libraries(DiscoverNotifierDeclarativePlugin
    KF5::Notifications
    KF5::ConfigCore
    KF5::I18n
    KF5::KIOWidgets
    Qt5::Qml

    Discover::Notifiers
)
install(TARGETS DiscoverNotifierDeclarativePlugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/discovernotifier)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/discovernotifier)

find_package(KF5Plasma)
if(KF5Plasma_FOUND)
    plasma_install_package(plasmoid org.kde.discovernotifier)
endif()
