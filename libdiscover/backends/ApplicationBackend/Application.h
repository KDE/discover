/***************************************************************************
 *   Copyright © 2010-2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QStringList>

#include <KService>

#include <AppstreamQt/component.h>
#include <QApt/Package>

#include "discovercommon_export.h"
#include "resources/AbstractResource.h"

class KJob;
class KConfig;
namespace QApt {
    class Backend;
}

class DISCOVERCOMMON_EXPORT Application : public AbstractResource
{
Q_OBJECT
public:
    explicit Application(const Appstream::Component &component, QApt::Backend *backend);
    explicit Application(QApt::Package *package, QApt::Backend *backend);

    QString name() override;
    QString comment() override;
    QApt::Package *package();
    QVariant icon() const override;
    QStringList mimetypes() const override;
    QStringList categories() override;
    QString license() override;
    QUrl screenshotUrl() override;
    QUrl thumbnailUrl() override;
    QList< PackageState > addonsInformation() override;
    bool isValid() const;
    bool isTechnical() const override;
    QString packageName() const override;

    //QApt::Package forwarding
    QUrl homepage() override;
    QString longDescription() override;
    QString installedVersion() const override;
    QString availableVersion() const override;
    QString sizeDescription() override;
    QString origin() const override;
    int size() override;

    bool hasScreenshot() const { return m_sourceHasScreenshot; }
    void setHasScreenshot(bool has);
    
    void clearPackage();
    QVector<KService::Ptr> findExecutables() const;
    QStringList executables() const override;
    
    /** Used to trigger the stateChanged signal from the ApplicationBackend */
    void emitStateChanged();
    
    void invokeApplication() const override;
    
    bool canExecute() const override;
    QString section() override;
    
    State state() override;
    void fetchScreenshots() override;
    void fetchChangelog() override;
    
    bool isFromSecureOrigin() const;

private Q_SLOTS:
    void processChangelog(KJob*);
    void downloadingScreenshotsFinished(KJob*);

private:
    QApt::Backend *backend() const;
    QStringList findProvides(Appstream::Provides::Kind kind) const;
    QString buildDescription(const QByteArray& data, const QString& source);
    
    const Appstream::Component m_data;
    QApt::Package *m_package;
    QString m_packageName;

    bool m_isValid;
    bool m_isTechnical;
    bool m_isExtrasApp;
    bool m_sourceHasScreenshot;

    QApt::PackageList addons();
};

#endif
