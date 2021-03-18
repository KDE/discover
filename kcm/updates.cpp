/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updates.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <updatesdata.h>
#include <updatessettings.h>

K_PLUGIN_FACTORY_WITH_JSON(UpdatesFactory, "kcm_updates.json", registerPlugin<Updates>(); registerPlugin<UpdatesData>();)

Updates::Updates(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent)
    , m_data(new UpdatesData(this))
{
    Q_UNUSED(args)

    qmlRegisterAnonymousType<UpdatesSettings>("org.kde.discover.updates", 1);

    setAboutData(new KAboutData(QStringLiteral("kcm_updates"),
                                i18n("Software Updates"),
                                QStringLiteral("1.0"),
                                i18n("Configure software update settings"),
                                KAboutLicense::LGPL));
}

Updates::~Updates() = default;
UpdatesSettings *Updates::updatesSettings() const
{
    return m_data->settings();
}

#include "updates.moc"
