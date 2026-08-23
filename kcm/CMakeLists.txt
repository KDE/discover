#SPDX-FileCopyrightText: (C) 2020 Aleix Pol Gonzalzez <aleixpol@kde.org>
#SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_updates\")

kcmutils_add_qml_kcm(kcm_updates SOURCES updates.cpp updates.h)
kcmutils_generate_module_data(kcm_updates
    MODULE_DATA_HEADER discoverdata.h
    MODULE_DATA_CLASS_NAME DiscoverData
    SETTINGS_HEADERS discoversettings.h
    SETTINGS_CLASSES DiscoverSettings
)
kcmutils_generate_module_data(kcm_updates
    MODULE_DATA_HEADER updatesdata.h
    MODULE_DATA_CLASS_NAME UpdatesData
    SETTINGS_HEADERS updatessettings.h
    SETTINGS_CLASSES UpdatesSettings
)

kconfig_add_kcfg_files(kcm_updates updatessettings.kcfgc GENERATE_MOC)
kconfig_add_kcfg_files(kcm_updates discoversettings.kcfgc GENERATE_MOC)

target_link_libraries(kcm_updates PRIVATE
    KF6::I18n
    KF6::ConfigGui
    KF6::KCMUtilsQuick
)

if (BUILD_HoloBackend)
    target_compile_definitions(kcm_updates PRIVATE WITH_HOLO_BACKEND)
endif()

if (BUILD_RpmOstreeBackend)
    target_compile_definitions(kcm_updates PRIVATE WITH_OSTREE_BACKEND)
endif()

if (BUILD_SystemdSysupdateBackend)
    target_compile_definitions(kcm_updates PRIVATE WITH_SYSUPDATE_BACKEND)

    add_library(discover_sysupdate_config STATIC transferconfig.cpp transferconfig.h)
    set_target_properties(discover_sysupdate_config PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(discover_sysupdate_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(discover_sysupdate_config PUBLIC Qt::Core KF6::CoreAddons)

    target_sources(kcm_updates PRIVATE
        imageversions.cpp
        imageversions.h
    )

    target_link_libraries(kcm_updates PRIVATE
        Qt::DBus
        KF6::AuthCore
        discover_sysupdate_config
    )

    add_subdirectory(helper)
endif()

install(FILES discoversettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME discoversettings-kcm.kcfg)
install(FILES updatessettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME discover-updates.kcfg)
