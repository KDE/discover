/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KNSCore/Entry>
#include <QPointer>
#include <attica/content.h>
#include <resources/AbstractResource.h>

#include <optional>

#include "discovercommon_export.h"

class KNSBackend;
class DISCOVERCOMMON_EXPORT KNSResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit KNSResource(const KNSCore::Entry &c, QStringList categories, KNSBackend *parent);
    ~KNSResource() override;

    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    bool hasCategory(const QString &category) const override;
    QUrl homepage() override;
    QJsonArray licenses() override;
    QString longDescription() override;
    QList<PackageState> addonsInformation() override
    {
        return QList<PackageState>();
    }
    QString availableVersion() const override;
    QString installedVersion() const override;
    QString origin() const override;
    QString displayOrigin() const override;
    QString section() override;
    void fetchScreenshots() override;
    quint64 size() override;
    void fetchChangelog() override;
    QStringList extends() const override;
    AbstractResource::Type type() const override
    {
        return Addon;
    }
    QString author() const override;

    KNSBackend *knsBackend() const;

    void setEntry(const KNSCore::Entry &entry);
    KNSCore::Entry entry() const;

    bool canExecute() const override;
    void invokeApplication() const override;

    QUrl url() const override;
    QString executeLabel() const override;
    QString sourceIcon() const override
    {
        return QStringLiteral("get-hot-new-stuff");
    }
    QDate releaseDate() const override;
    QVector<int> linkIds() const;
    QUrl donationURL() override;

    Rating rating() const override;

private:
    const QStringList m_categories;
    KNSCore::Entry m_entry;
    KNSCore::Entry::Status m_lastStatus;
    mutable std::optional<Rating> m_rating;
};
