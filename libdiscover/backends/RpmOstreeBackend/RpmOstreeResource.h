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
    explicit RpmOstreeResource(QString name,
                               QString baseVersion,
                               QString checkSum,
                               QString signature,
                               QString layeredPackages,
                               QString localPackages,
                               QString origin,
                               qulonglong timestamp,
                               RpmOstreeBackend *parent);
    QString appstreamId() const override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    QStringList categories() override
    {
        return QStringList();
    }
    QUrl homepage() override;
    QJsonArray licenses() override;
    QString longDescription() override;
    QList<PackageState> addonsInformation() override
    {
        return QList<PackageState>();
    }
    bool isRemovable() const override
    {
        return false;
    }
    QString availableVersion() const override;
    void setNewVersion(QString);
    QString installedVersion() const override;
    QString origin() const override;
    QString section() override;
    void fetchScreenshots() override {};
    int size() override;
    void fetchChangelog() override {};
    QStringList extends() const override
    {
        return QStringList();
    }
    AbstractResource::Type type() const override
    {
        return Addon;
    }
    QString author() const override;
    bool canExecute() const override;
    void invokeApplication() const override {};

    QUrl url() const override;
    QString executeLabel() const override;
    QString sourceIcon() const override
    {
        return QStringLiteral("application-x-rpm");
    }
    QDate releaseDate() const override;
    QUrl donationURL() override;
    void setState(AbstractResource::State);
    void setRemoteRefsList(QStringList remoteRefs);
    static const QStringList m_objects;

    /*
     * It is called when the user clicks on the button to switch to a new kinoite refs and
     * it emits buttonPressed() so that the corresponding system upgrade function
     * is executed in the backend.
     */
    Q_SCRIPTABLE void rebaseToNewVersion();

    /*
     * Filtering the kinoite remote refs list to get the recent kinoite refs
     * and display it in the button of the current running deployment resource.
     */
    Q_SCRIPTABLE QString getRecentRemoteRefs();

    /*
     * Indicating if there is a new refs or not and use it to enable/disable the button
     * which telling the user that there is a new kinoite refs to switch to.
     */
    Q_SCRIPTABLE bool isRecentRefsAvaliable();

Q_SIGNALS:

    /*
     * It is emitted when the user wants to switch to a new avaliable kinoite refs.
     */
    void buttonPressed(QString);

private:
    QString m_deploymentName;
    QString m_version;
    QString m_newVersion;
    QString m_checkSum;
    QString m_signature;
    QString m_layeredPackages;
    QString m_localPackages;
    AbstractResource::State m_state;
    QStringList m_remoteRefsList;
    QString m_currentRefs;
    QString m_recentRefs;
    QDate m_releaseDate;
};

#endif
