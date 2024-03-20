// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <resources/AbstractResourcesBackend.h>

#include <KOSRelease>

class KDELinuxResource : public AbstractResource
{
    Q_OBJECT
public:
    explicit KDELinuxResource(const QString &version, AbstractResourcesBackend *parent);

    [[nodiscard]] QString packageName() const override;
    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString comment() override;
    [[nodiscard]] QVariant icon() const override;
    [[nodiscard]] bool canExecute() const override;
    [[nodiscard]] State state() override;
    [[nodiscard]] bool hasCategory(const QString &category) const override;
    [[nodiscard]] Type type() const override;
    [[nodiscard]] quint64 size() override;
    [[nodiscard]] QJsonArray licenses() override;
    [[nodiscard]] QString installedVersion() const override;
    [[nodiscard]] QString availableVersion() const override;
    [[nodiscard]] QString longDescription() override;
    [[nodiscard]] QString origin() const override;
    [[nodiscard]] QString section() override;
    [[nodiscard]] QString author() const override;
    [[nodiscard]] QList<PackageState> addonsInformation() override;
    [[nodiscard]] QString sourceIcon() const override;
    [[nodiscard]] QDate releaseDate() const override;
    void fetchChangelog() override;
    void invokeApplication() const override;

private:
    QString m_version;
    KOSRelease m_osrelease;
};
