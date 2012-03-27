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

#include <KUrl>
#include <KService>

#include <LibQApt/Package>

#include "libmuonprivate_export.h"

namespace QApt {
    class Backend;
}

class MUONPRIVATE_EXPORT Application : public QObject
{
Q_OBJECT
Q_PROPERTY(QString name READ name CONSTANT)
Q_PROPERTY(QString untranslatedName READ untranslatedName CONSTANT)
Q_PROPERTY(QString comment READ comment CONSTANT)
Q_PROPERTY(QString icon READ icon CONSTANT)
Q_PROPERTY(QString mimetypes READ mimetypes CONSTANT)
Q_PROPERTY(QString menuPath READ menuPath CONSTANT)
Q_PROPERTY(QString categories READ categories CONSTANT)
Q_PROPERTY(QString homepage READ homepage CONSTANT)
Q_PROPERTY(QString longDescription READ longDescription CONSTANT)
Q_PROPERTY(QString license READ license CONSTANT)
Q_PROPERTY(QString installedVersion READ installedVersion CONSTANT)
Q_PROPERTY(QString availableVersion READ availableVersion CONSTANT)
Q_PROPERTY(QString sizeDescription READ sizeDescription NOTIFY installChanged)
Q_PROPERTY(QString origin READ origin CONSTANT)
Q_PROPERTY(bool isValid READ isValid CONSTANT)
Q_PROPERTY(bool isTechnical READ isTechnical CONSTANT)
Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY installChanged)
Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY installChanged)
Q_PROPERTY(int usageCount READ usageCount CONSTANT)
public:
    friend class TransactionListener;

    explicit Application(const QString &fileName, QApt::Backend *backend);
    explicit Application(QApt::Package *package, QApt::Backend *backend);
    ~Application();

    QString name();
    QString untranslatedName();
    QString comment();
    QApt::Package *package();
    QString icon() const;
    QString mimetypes() const;
    QString menuPath();
    QString categories();
    QString license();
    Q_SCRIPTABLE QUrl screenshotUrl() { return screenshotUrl(QApt::Screenshot); }
    KUrl screenshotUrl(QApt::ScreenshotType type);
    QApt::PackageList addons();
    bool isValid() const;
    bool isTechnical() const;
    int usageCount();
    QString packageName() const;

    Q_SCRIPTABLE QByteArray getField(const QByteArray &field) const;
    Q_SCRIPTABLE QHash<QByteArray, QByteArray> desktopContents();

    //QApt::Package forwarding
    bool isInstalled() const;
    QString homepage() const;
    QString longDescription() const;
    QString installedVersion() const;
    QString availableVersion() const;
    QString sizeDescription();
    QString origin() const;
    bool canUpgrade();

    void clearPackage();
    QVector<KService::Ptr> executables();
    
    /** Used to trigger the installChanged signal from the ApplicationBackend */
    void emitInstallChanged();
signals:
    void installChanged();

private:
    void populateZeitgeistInfo();
    QVector<QPair<QString, QString> > locateApplication(const QString &_relPath, const QString &menuId) const;

    QString m_fileName;
    QHash<QByteArray, QByteArray> m_data;
    QApt::Backend *m_backend;
    QApt::Package *m_package;
    QString m_packageName;

    bool m_isValid;
    bool m_isTechnical;
    int m_usageCount;
};

#endif
