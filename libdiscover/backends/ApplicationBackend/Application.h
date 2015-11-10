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
Q_PROPERTY(QString menuPath READ menuPath CONSTANT)
public:
    explicit Application(const QString &fileName, QApt::Backend *backend);
    explicit Application(QApt::Package *package, QApt::Backend *backend);

    QString name();
    QString untranslatedName();
    QString comment();
    QApt::Package *package();
    QString icon() const;
    QStringList mimetypes() const;
    QString menuPath();
    QStringList categories();
    QString license();
    QUrl screenshotUrl();
    QUrl thumbnailUrl();
    virtual QList< PackageState > addonsInformation();
    bool isValid() const;
    bool isTechnical() const;
    QString packageName() const;

    //QApt::Package forwarding
    QUrl homepage();
    QString longDescription();
    QString installedVersion() const;
    QString availableVersion() const;
    QString sizeDescription();
    QString origin() const;
    int size();

    bool hasScreenshot() const { return m_sourceHasScreenshot; }
    void setHasScreenshot(bool has);
    
    void clearPackage();
    QVector<KService::Ptr> findExecutables() const;
    virtual QStringList executables() const;
    
    /** Used to trigger the stateChanged signal from the ApplicationBackend */
    void emitStateChanged();
    
    void invokeApplication() const;
    
    bool canExecute() const;
    QString section();
    
    virtual State state();
    virtual void fetchScreenshots();
    virtual void fetchChangelog();
    
    bool isFromSecureOrigin() const;
    QByteArray getField(const char* field, const QByteArray& defaultvalue = QByteArray()) const;

private slots:
    void processChangelog(KJob*);
    void downloadingScreenshotsFinished(KJob*);

private:
    QString buildDescription(const QByteArray& data, const QString& source);
    
    QSharedPointer<KConfig> m_data;
    QApt::Backend *m_backend;
    QApt::Package *m_package;
    QByteArray m_packageName;

    bool m_isValid;
    bool m_isTechnical;
    bool m_isExtrasApp;
    bool m_sourceHasScreenshot;

    QApt::PackageList addons();
    QVector<QPair<QString, QString> > locateApplication(const QString &_relPath, const QString &menuId) const;
    bool hasField(const char* field) const;
};

#endif
