add_subdirectory(tests)

set(flatpak-backend_SRCS
    FlatpakResource.cpp
    FlatpakBackend.cpp
    FlatpakFetchDataJob.cpp
    FlatpakSourcesBackend.cpp
    FlatpakJobTransaction.cpp
    FlatpakTransactionThread.cpp
)

add_library(flatpak-backend MODULE ${flatpak-backend_SRCS})
target_link_libraries(flatpak-backend Qt::Core Qt::Widgets Qt::Concurrent KF5::CoreAddons KF5::ConfigCore Discover::Common AppStreamQt PkgConfig::Flatpak)

if (NOT Flatpak_VERSION VERSION_LESS 1.1.2)
    target_compile_definitions(flatpak-backend PRIVATE -DFLATPAK_VERBOSE_PROGRESS -DFLATPAK_LIST_UNUSED_REFS)
endif()

install(TARGETS flatpak-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
install(FILES flatpak-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

add_library(FlatpakNotifier MODULE FlatpakNotifier.cpp)
target_link_libraries(FlatpakNotifier Discover::Notifiers Qt::Concurrent PkgConfig::Flatpak)
set_target_properties(FlatpakNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS FlatpakNotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)
install(PROGRAMS org.kde.discover-flatpak.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install(FILES org.kde.discover.flatpak.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

ecm_install_icons(
    ICONS
        sc-apps-flatpak-discover.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
