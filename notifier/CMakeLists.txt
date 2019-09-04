add_definitions(-DTRANSLATION_DOMAIN=\"plasma-discover-notifier\")

add_executable(DiscoverNotifier BackendNotifierFactory.cpp DiscoverNotifier.cpp main.cpp)
target_link_libraries(DiscoverNotifier
    KF5::Notifications
    KF5::I18n
    KF5::KIOWidgets
    KF5::Crash
    KF5::DBusAddons

    Discover::Notifiers
)
set_target_properties(DiscoverNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
install(TARGETS DiscoverNotifier DESTINATION ${KDE_INSTALL_LIBEXECDIR})

set(DesktopExec "${KDE_INSTALL_FULL_LIBEXECDIR}/DiscoverNotifier")
configure_file(org.kde.discover.notifier.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
