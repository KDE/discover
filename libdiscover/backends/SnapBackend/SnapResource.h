/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SNAPRESOURCE_H
#define SNAPRESOURCE_H

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
    Q_PROPERTY(QStringList objects MEMBER m_objects CONSTANT)
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

    QString channel() const;
    void setChannel(const QString &channel);

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

    QSharedPointer<QSnapdSnap> m_snap;
    mutable QVariant m_icon;
    static const QStringList m_objects;
};

#endif // SNAPRESOURCE_H
