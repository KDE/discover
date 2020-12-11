#SPDX-FileCopyrightText: (C) 2020 Aleix Pol Gonzalzez <aleixpol@kde.org>
#SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_updates\")

kcmutils_generate_module_data(
    kcm_updates_PART_SRCS
    MODULE_DATA_HEADER updatesdata.h
    MODULE_DATA_CLASS_NAME UpdatesData
    SETTINGS_HEADERS updatessettings.h
    SETTINGS_CLASSES UpdatesSettings
)

kconfig_add_kcfg_files(kcm_updates_PART_SRCS updatessettings.kcfgc GENERATE_MOC)
add_library(kcm_updates MODULE updates.cpp ${kcm_updates_PART_SRCS})

target_link_libraries(kcm_updates
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
)

kcoreaddons_desktop_to_json(kcm_updates "kcm_updates.desktop")

install(TARGETS kcm_updates DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)
install(FILES kcm_updates.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
kpackage_install_package(package kcm_updates kcms)
