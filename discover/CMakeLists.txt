add_subdirectory(icons)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

ecm_qt_declare_logging_category(plasma_discover_SRCS HEADER discover_debug.h IDENTIFIER DISCOVER_LOG CATEGORY_NAME org.kde.plasma.discover DESCRIPTION "discover" EXPORT DISCOVER)
kconfig_add_kcfg_files(plasma_discover_SRCS discoversettings.kcfgc GENERATE_MOC)

if (TARGET KF6::UserFeedbackCore)
    kconfig_add_kcfg_files(plasma_discover_SRCS plasmauserfeedback.kcfgc GENERATE_MOC)
endif()

# Avoid problems with sign conversion by warning about them. Selectively in this file because we know there's been
# problems in the past. Ideally we'd move towards applying this everywhere.
# ES.100: Don’t mix signed and unsigned arithmetic
# https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-mix
# https://bugs.kde.org/show_bug.cgi?id=513676
set_source_files_properties(DiscoverObject.cpp PROPERTIES COMPILE_OPTIONS "-Wsign-conversion")

add_executable(plasma-discover ${plasma_discover_SRCS}
    main.cpp
    DiscoverObject.cpp
    DiscoverDeclarativePlugin.cpp
    BigscreenApp.cpp

    AbstractAppsModel.cpp
    OdrsAppsModel.cpp
    FeaturedModel.cpp
    LimitedRowCountProxyModel.cpp
    UnityLauncher.cpp
    ReadFile.cpp
    PowerManagementInterface.cpp

    DiscoverObject.h
    DiscoverDeclarativePlugin.h
    BigscreenApp.h

    FeaturedModel.h
    UnityLauncher.h
    ReadFile.h


    resources.qrc
    RefreshNotifier.cpp
)
add_executable(Plasma::Discover ALIAS plasma-discover)
set_target_properties(plasma-discover PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover PUBLIC
                                     KF6::CoreAddons
                                     KF6::Crash
                                     KF6::DBusAddons
                                     KF6::I18n
                                     KF6::ConfigGui
                                     KF6::KIOCore
                                     KF6::KIOGui
                                     KF6::WindowSystem
                                     KF6::Notifications
                                     KF6::JobWidgets
                                     KF6::StatusNotifierItem
                                     KF6::I18nQml
                                     KirigamiApp
                                     Qt::Widgets
                                     Qt::Quick
                                     Qt::QuickControls2
                                     Discover::Common
)

# Plasma Bigscreen (TV) interface, shown when the runtime platform is "mediacenter" or with --ui bigscreen
ecm_find_qmlmodule(org.kde.bigscreen.ui 1.0)
set_package_properties(org.kde.bigscreen.ui-QMLModule PROPERTIES
    TYPE RUNTIME
    PURPOSE "Required at runtime by Discover for Plasma Bigscreen"
)

ecm_add_qml_module(plasma-discover
    URI org.kde.discover.bigscreen
    GENERATE_PLUGIN_SOURCE
    NO_IMPORT_SCAN
    DEPENDENCIES
        QtQuick
        org.kde.kirigami
)

set(bigscreen_QML
    qml/bigscreen/AppBackdrop.qml
    qml/bigscreen/AppCard.qml
    qml/bigscreen/AppGrid.qml
    qml/bigscreen/AppPage.qml
    qml/bigscreen/AppRail.qml
    qml/bigscreen/AppTile.qml
    qml/bigscreen/DescriptionPage.qml
    qml/bigscreen/HomePage.qml
    qml/bigscreen/InstallButton.qml
    qml/bigscreen/Main.qml
    qml/bigscreen/ScreenshotPage.qml
    qml/bigscreen/SearchPage.qml
    qml/bigscreen/SettingsPage.qml
    qml/bigscreen/StateBadge.qml
)
foreach(file IN LISTS bigscreen_QML)
    get_filename_component(alias ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

ecm_target_qml_sources(plasma-discover SOURCES ${bigscreen_QML})

if (TARGET Qt::WebView)
    target_link_libraries(plasma-discover PUBLIC Qt::WebView)
    target_compile_definitions(plasma-discover PUBLIC -DWITH_QTWEBVIEW=1)
else()
    target_compile_definitions(plasma-discover PUBLIC -DWITH_QTWEBVIEW=0)
endif()

if (TARGET KF6::UserFeedbackCore)
    target_link_libraries(plasma-discover PRIVATE KF6::UserFeedbackCore)
    target_compile_definitions(plasma-discover PRIVATE WITH_FEEDBACK=1)
endif()

install(TARGETS plasma-discover ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# if (BUILD_DummyBackend)
#     target_compile_definitions(plasma-discover PRIVATE $<$<CONFIG:Debug>:QT_QML_DEBUG=1>)
# endif()

# Standard desktop file accepts local files as input.
set(DesktopNoDisplay "false")
find_program(DPKG dpkg)
find_program(RPM rpm)
set(DesktopMimeType "")
if(DPKG)
     set(DesktopMimeType "${DesktopMimeType}application/vnd.debian.binary-package;")
endif()
if(RPM)
     set(DesktopMimeType "${DesktopMimeType}application/x-rpm;")
endif()
if(Flatpak_FOUND)
  set(DesktopMimeType "${DesktopMimeType}application/vnd.flatpak;application/vnd.flatpak.repo;application/vnd.flatpak.ref;")
endif()
set(DesktopExec "plasma-discover %F")
# Make sure the main entry doesn't have alias
set(DesktopAliasFor "")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

# All hidden helper entries below launch Discover, map them to the main entry
set(DesktopAliasFor "X-KDE-AliasFor=org.kde.discover")

# Support appstream:// URI
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/appstream;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

if(Flatpak_FOUND)
    # support Flatpak flatpak+https URI
    set(DesktopNoDisplay "true")
    set(DesktopMimeType "x-scheme-handler/flatpak+https;")
    set(DesktopExec "plasma-discover %U")
    configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.flatpak.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.flatpak.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()

# support snap:/ URI
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/snap;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.snap.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.snap.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

if(EXISTS "/etc/debian_version")
    set(DesktopNoDisplay "true")
    set(DesktopMimeType "x-scheme-handler/apt")
    set(DesktopExec "plasma-discover %U")
    configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()

# Support local AppStream files
set(DesktopNoDisplay "true")
set(DesktopMimeType "application/x-freedesktop-appstream-component")
set(DesktopExec "plasma-discover --backends appstream-preview %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.appstreampreview.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.appstreampreview.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

install(FILES plasmadiscoverui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/plasmadiscover)
install(FILES org.kde.discover.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
install(FILES discoversettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
