/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef ABSTRACTRESOURCE_H
#define ABSTRACTRESOURCE_H

#include <QtCore/QObject>
#include <QUrl>

#include "libmuonprivate_export.h"
#include "PackageState.h"

class MUONPRIVATE_EXPORT AbstractResource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString packageName READ packageName CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(bool canExecute READ canExecute CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString category READ categories CONSTANT)
    Q_PROPERTY(bool isTechnical READ isTechnical CONSTANT)
    Q_PROPERTY(QUrl homepage READ homepage CONSTANT)
    Q_PROPERTY(QUrl thumbnailUrl READ thumbnailUrl CONSTANT)
    Q_PROPERTY(QUrl screenshotUrl READ screenshotUrl CONSTANT)
    Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY stateChanged)
    Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY stateChanged)
    Q_PROPERTY(QString license READ license CONSTANT)
    Q_PROPERTY(QString longDescription READ longDescription CONSTANT)
    Q_PROPERTY(QString origin READ origin CONSTANT)
    Q_PROPERTY(QString sizeDescription READ sizeDescription NOTIFY stateChanged)
    Q_PROPERTY(QString installedVersion READ installedVersion CONSTANT)
    Q_PROPERTY(QString availableVersion READ availableVersion CONSTANT)
    Q_PROPERTY(QString section READ section CONSTANT)
    Q_PROPERTY(int popcon READ popularityContest CONSTANT)
    Q_PROPERTY(QString mimetypes READ mimetypes CONSTANT)
    public:
        enum State {
            Broken,
            None,
            Installed,
            Upgradeable
        };
        Q_ENUMS(State)
        
        explicit AbstractResource(QObject* parent = 0);
        
        ///used as internal identification of a resource
        virtual QString packageName() const = 0;
        
        ///resource name to be displayed
        virtual QString name() = 0;
        
        ///short description of the resource
        virtual QString comment() = 0;
        
        ///xdg-compatible icon name to represent the resource
        virtual QString icon() const = 0;
        
        ///@returns whether invokeApplication makes something
        /// false if not overriden
        virtual bool canExecute() const;
        
        ///executes the resource, if applies.
        Q_SCRIPTABLE virtual void invokeApplication() const;
        
        virtual State state() = 0;
        
        virtual QString categories() = 0;
        
        ///@returns a URL that points to the content
        virtual QUrl homepage() const = 0;
        
        virtual bool isTechnical() const;

        virtual QUrl thumbnailUrl() = 0;
        virtual QUrl screenshotUrl() = 0;
        
        virtual QString sizeDescription() = 0;
        virtual QString license() = 0;
        
        virtual QString installedVersion() const = 0;
        virtual QString availableVersion() const = 0;
        virtual QString longDescription() const = 0;
        
        virtual QString origin() const = 0;
        virtual QString section() = 0;
        
        ///@returns what kind of mime types the resource can consume
        virtual QString mimetypes() const;
        
        /** Popularity rating by Ubuntu.
         * Maybe we should deprecate? we don't really have a scale for this
         */
        virtual int popularityContest() const;
        
        bool canUpgrade();
        bool isInstalled();
        
        virtual QList<PackageState> addonsInformation() = 0;

    public slots:
        virtual void fetchScreenshots();

    signals:
        void stateChanged();
        
        ///response to the fetchScreenshots method
        ///@p thumbnails and @p screenshots should have the same number of elements
        void screenshotsFetched(const QList<QUrl>& thumbnails, const QList<QUrl>& screenshots);
};

#endif // ABSTRACTRESOURCE_H
