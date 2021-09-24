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
class RpmOstreeResource : public AbstractResource
{
    Q_OBJECT
    Q_PROPERTY(QStringList objects MEMBER m_objects CONSTANT)
public:
    explicit RpmOstreeResource(const QVariantMap &, RpmOstreeBackend *);

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
    void fetchScreenshots() override {};
    quint64 size() override;
    QString sizeDescription() override;
    void fetchChangelog() override {};
    QStringList extends() const override;
    AbstractResource::Type type() const override;
    QString author() const override;
    bool canExecute() const override;
    void invokeApplication() const override {};
    QUrl url() const override;
    QString sourceIcon() const override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QDate releaseDate() const override;
    QUrl donationURL() override;

    void setState(AbstractResource::State);
    void fetchRemoteRefs();
    void setNewVersion(QString);

    static const QStringList m_objects;

    /*
     * It is called when the user clicks on the button to switch to a new kinoite refs and
     * it emits buttonPressed() so that the corresponding system upgrade function
     * is executed in the backend.
     */
    Q_SCRIPTABLE void rebaseToNewVersion();

    /** Returns the next major version for the current deployment. */
    Q_SCRIPTABLE QString getNextMajorVersion();

    /**
     * Returns true only if there is a newer ref available in the current remote.
     * TODO: Only display this button when the new version is stable (and not just branched as it works today.
     */
    Q_SCRIPTABLE bool isNextMajorVersionAvailable();

    /** Returns if a given deployment is the currently booted deployment. */
    Q_SCRIPTABLE bool isBooted();

Q_SIGNALS:

    /** Signal emitted when the user requests the rebase to a newer version */
    void buttonPressed(QString);

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

    AbstractResource::State m_state;

    QString m_newVersion;
    QString m_nextMajorVersion;
    QStringList m_remoteRefs;
    QString m_currentRefs;
};

#endif
