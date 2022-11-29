/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef OSTREERPMRESOURCE_H
#define OSTREERPMRESOURCE_H

#include <resources/AbstractResource.h>

class RpmOstreeBackend;
class QAbstractItemModel;

/*
 * Represents an ostree deployment (an installed version of the system) as a
 * resource in Discover.
 */
class RpmOstreeResource : public AbstractResource
{
    Q_OBJECT
public:
    RpmOstreeResource(const QVariantMap &map, RpmOstreeBackend *parent);

    QString appstreamId() const override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    Q_SCRIPTABLE QString packageName() const override;
    QStringList categories() override;
    QJsonArray licenses() override;
    QString longDescription() override;
    QList<PackageState> addonsInformation() override;
    bool isRemovable() const override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QString origin() const override;
    QString section() override;
    void fetchScreenshots() override{};
    quint64 size() override;
    QString sizeDescription() override;
    void fetchChangelog() override{};
    QStringList extends() const override;
    AbstractResource::Type type() const override;
    QString author() const override;
    bool canExecute() const override;
    void invokeApplication() const override{};
    QUrl url() const override;
    QString sourceIcon() const override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QDate releaseDate() const override;
    QUrl donationURL() override;

    void setState(AbstractResource::State);

    /* Get the current version */
    QString version();

    /* Set the target version for updates */
    void setNewVersion(const QString &newVersion);

    /* Get the target version for updates */
    QString getNewVersion() const;

    /* Validate and set the target major version for rebase */
    bool setNewMajorVersion(const QString &newMajorVersion);

    /* Returns the next major version for the deployment */
    QString getNextMajorVersion() const;

    /* Returns the ostree ref for the next major version for the deployment */
    QString getNextMajorVersionRef() const;

    /* Returns if a given deployment is the currently booted deployment */
    Q_SCRIPTABLE bool isBooted();

    /* Returns if a given deployment is currently pending */
    Q_SCRIPTABLE bool isPending();


private:
    QString m_name;
    QString m_variant;
    QString m_osname;
    QString m_version;
    QDate m_timestamp;
    QString m_origin;
    QString m_remote;
    QString m_branch;
    QString m_branchName;
    QString m_branchVersion;
    QString m_branchArch;
    QString m_branchVariant;
    QString m_appstreamid;
    bool m_booted;
    bool m_pinned;
    bool m_pending;
    QStringList m_requested_base_local_replacements;
    QStringList m_requested_base_removals;
    QStringList m_requested_local_packages;
    QStringList m_requested_modules;
    QStringList m_requested_packages;
    QString m_checksum;

    AbstractResource::State m_state;

    QString m_newVersion;
    QString m_nextMajorVersion;
};

#endif
