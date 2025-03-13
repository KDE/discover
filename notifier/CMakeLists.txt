add_definitions(-DTRANSLATION_DOMAIN=\"plasma-discover-notifier\")

kconfig_add_kcfg_files(notifier_SRCS ../kcm/updatessettings.kcfgc GENERATE_MOC)

qt_add_dbus_interface(notifier_SRCS org.freedesktop.login1.Manager.xml Login1ManagerInterface)

add_executable(DiscoverNotifier
    BackendNotifierFactory.cpp
    DiscoverNotifier.cpp
    NotifierItem.cpp
    UnattendedUpdates.cpp
    main.cpp

    ${notifier_SRCS}
)

target_link_libraries(DiscoverNotifier
    KF6::Notifications
    KF6::JobWidgets
    KF6::StatusNotifierItem
    KF6::I18n
    KF6::KIOGui
    KF6::Crash
    KF6::DBusAddons
    KF6::ConfigGui
    KF6::IdleTime

    Discover::Notifiers
)

ecm_qt_declare_logging_category(DiscoverNotifier
    HEADER debug.h
    IDENTIFIER NOTIFIER
    CATEGORY_NAME org.kde.plasma.discover.notifier
    DESCRIPTION "Plasma Discover Notifier"
    EXPORT DISCOVER
)

set_target_properties(DiscoverNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
install(TARGETS DiscoverNotifier DESTINATION ${KDE_INSTALL_LIBEXECDIR})

set(DesktopExec "${KDE_INSTALL_FULL_LIBEXECDIR}/DiscoverNotifier")
configure_file(org.kde.discover.notifier.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.notifier.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
