# SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

# Plasma Bigscreen (TV) interface for Discover

ecm_find_qmlmodule(org.kde.bigscreen.ui 1.0)
set_package_properties(org.kde.bigscreen.ui-QMLModule PROPERTIES
    TYPE RUNTIME
    PURPOSE "Required at runtime by Discover for Plasma Bigscreen"
)

add_executable(plasma-discover-bigscreen main.cpp)
set_target_properties(plasma-discover-bigscreen PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover-bigscreen PRIVATE
    discover-ui-common
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::DBusAddons
    KF6::I18n
    KF6::I18nQml
    KF6::WindowSystem
    KirigamiApp
    Qt::Widgets
    Qt::Quick
    Discover::Common
)

ecm_add_qml_module(plasma-discover-bigscreen
    URI org.kde.discover.bigscreen
    GENERATE_PLUGIN_SOURCE
    NO_IMPORT_SCAN
    DEPENDENCIES
        QtQuick
        org.kde.kirigami
)

set(bigscreen_QML
    qml/AppBackdrop.qml
    qml/AppCard.qml
    qml/AppGrid.qml
    qml/AppPage.qml
    qml/AppRail.qml
    qml/AppTile.qml
    qml/DescriptionPage.qml
    qml/HomePage.qml
    qml/InstallButton.qml
    qml/Main.qml
    qml/ScreenshotPage.qml
    qml/SearchPage.qml
    qml/SettingsPage.qml
    qml/StateBadge.qml
)
foreach(file IN LISTS bigscreen_QML)
    get_filename_component(alias ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

ecm_target_qml_sources(plasma-discover-bigscreen SOURCES ${bigscreen_QML})

install(TARGETS plasma-discover-bigscreen ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.discover.bigscreen.desktop DESTINATION ${KDE_INSTALL_APPDIR})
