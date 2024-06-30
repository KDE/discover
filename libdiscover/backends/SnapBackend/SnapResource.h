/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QJsonObject>
#include <QSharedPointer>
#include <Snapd/Snap>
#include <resources/AbstractResource.h>

class SnapBackend;
class QAbstractItemModel;
class QSnapdClient;

class SnapResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel WRITE setChannel NOTIFY channelChanged)
public:
    explicit SnapResource(QSharedPointer<QSnapdSnap> snap, AbstractResource::State state, SnapBackend *parent);
    ~SnapResource() override = default;

    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    QStringList categories() override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    AbstractResource::Type type() const override;
    bool canExecute() const override
    {
        return true;
    }

    /**
     * Tries to obtain a launchable desktop file for this snap, in this order:
     *
     * 1. Any app with the same name as the snap and a desktop file (the main app)
     * 2. The first app with a desktop file (the next best app)
     * 3. The expected desktop file for the main app (<snap_name>_<snap_name>.desktop)
     *
     * @return The fileName of the selected launchable desktop file.
     */
    QString launchableDesktopFile() const;

    /**
     * Launches a snap using its desktop file.
     *
     * If no desktop file is found, defaults back to `snap run`, which will fail in Ubuntu Core environments.
     */
    void invokeApplication() const override;

    void fetchChangelog() override;
    void fetchScreenshots() override;
    QString author() const override;
    QList<PackageState> addonsInformation() override
    {
        return {};
    }
    void setSnap(const QSharedPointer<QSnapdSnap> &snap);

    void setState(AbstractResource::State state);
    QString sourceIcon() const override
    {
        return QStringLiteral("snap");
    }

    QDate releaseDate() const override;

    Q_SCRIPTABLE QAbstractItemModel *plugs(QObject *parentC);
    Q_SCRIPTABLE QObject *channels(QObject *parent);
    QString appstreamId() const override;

    QStringList topObjects() const override;

    QString channel();
    void setChannel(const QString &channel);

    quint64 installedSize() const;
    quint64 downloadSize() const;
    void updateSizes();

    QUrl homepage() override;
    QUrl url() const override;

    QSharedPointer<QSnapdSnap> snap() const
    {
        return m_snap;
    }

Q_SIGNALS:
    void channelChanged(const QString &channel);
    void newSnap();

public:
    QSnapdClient *client() const;
    void refreshSnap();
    void gotIcon();
    AbstractResource::State m_state;
    quint64 m_installedSize;
    quint64 m_downloadSize;

    QSharedPointer<QSnapdSnap> m_snap;
    QString m_channel;
    mutable QVariant m_icon;
    static const QStringList s_topObjects;
};
