/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DUMMYRESOURCE_H
#define DUMMYRESOURCE_H

#include <resources/AbstractResource.h>

class AddonList;
class DummyResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit DummyResource(QString name, AbstractResource::Type type, AbstractResourcesBackend *parent);

    QList<PackageState> addonsInformation() override;
    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    QStringList categories() override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    AbstractResource::Type type() const override
    {
        return m_type;
    }
    bool canExecute() const override
    {
        return true;
    }
    void invokeApplication() const override;
    void fetchChangelog() override;
    void fetchScreenshots() override;
    QUrl url() const override;
    QString author() const override
    {
        return QStringLiteral("BananaPerson");
    }
    void setState(State state);
    void setSize(quint64 size)
    {
        m_size = size;
    }
    void setAddons(const AddonList &addons);

    void setAddonInstalled(const QString &addon, bool installed);
    QString sourceIcon() const override
    {
        return QStringLiteral("player-time");
    }
    QDate releaseDate() const override
    {
        return {};
    }

public:
    const QString m_name;
    AbstractResource::State m_state;
    QList<QUrl> m_screenshots;
    QList<QUrl> m_screenshotThumbnails;
    QString m_iconName;
    QList<PackageState> m_addons;
    const AbstractResource::Type m_type;
    quint64 m_size;
};

#endif // DUMMYRESOURCE_H
