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

#ifndef APPLICATIONUPDATES_H
#define APPLICATIONUPDATES_H
#include <QObject>
#include <LibQApt/Globals>

class Application;
class ApplicationUpdates : public QObject
{
    Q_OBJECT
    public:
        explicit ApplicationUpdates(QObject* parent = 0);
        Q_SCRIPTABLE void updateApplications(const QList< QObject* >& apps);
        Q_SCRIPTABLE void upgradeAll();
        
    signals:
        void progress(const QString& text, int percentage);
        void downloadMessage(int code, const QString& msg);
        void installMessage(const QString& msg);
        void updatesFinnished();
        
    public slots:
        void workerEvent(QApt::WorkerEvent);
};

#endif // APPLICATIONUPDATES_H
