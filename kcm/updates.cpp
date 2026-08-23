/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updates.h"

#include <QFile>
#include <qqml.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <discoverdata.h>

#include <updatesdata.h>

K_PLUGIN_FACTORY_WITH_JSON(UpdatesFactory, "kcm_updates.json", registerPlugin<Updates>(); registerPlugin<UpdatesData>();)

Updates::Updates(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
#ifdef WITH_SYSUPDATE_BACKEND
    , m_imageVersions(ImageVersions::isSupported() ? new ImageVersions(this) : nullptr)
#endif
    , m_data(new UpdatesData(this))
    , m_discoverData(new DiscoverData(this))
{
    qmlRegisterAnonymousType<UpdatesSettings>("org.kde.discover.updates", 1);
    qmlRegisterAnonymousType<DiscoverSettings>("org.kde.discover.updates", 1);
#ifdef WITH_SYSUPDATE_BACKEND
    qmlRegisterAnonymousType<ImageVersions>("org.kde.discover.updates", 1);

    if (m_imageVersions) {
        connect(m_imageVersions, &ImageVersions::configurationChanged, this, &Updates::settingsChanged);
    }
#endif
}

UpdatesSettings *Updates::updatesSettings() const
{
    return m_data->settings();
}

DiscoverSettings *Updates::discoverSettings() const
{
    return m_discoverData->settings();
}

bool Updates::mandatoryRebootAfterUpdate() const
{
#if defined(WITH_SYSUPDATE_BACKEND) || defined(WITH_HOLO_BACKEND)
    return true;
#elif defined WITH_OSTREE_BACKEND
    return QFile::exists(QStringLiteral("/run/ostree-booted"));
#else
    return false;
#endif
}

#ifdef WITH_SYSUPDATE_BACKEND
ImageVersions *Updates::imageVersions() const
{
    return m_imageVersions;
}

void Updates::load()
{
    KQuickManagedConfigModule::load();

    if (m_imageVersions) {
        m_imageVersions->load();
    }
}

void Updates::save()
{
    KQuickManagedConfigModule::save();

    if (m_imageVersions) {
        m_imageVersions->save();
    }
}

void Updates::defaults()
{
    KQuickManagedConfigModule::defaults();

    if (m_imageVersions) {
        m_imageVersions->defaults();
    }
}

bool Updates::isSaveNeeded() const
{
    return (m_imageVersions && m_imageVersions->isSaveNeeded());
}

bool Updates::isDefaults() const
{
    return (!m_imageVersions || m_imageVersions->isDefaults());
}
#endif

#include "moc_updates.cpp"
#include "updates.moc"
