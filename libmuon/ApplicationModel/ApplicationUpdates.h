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
#include <resources/AbstractBackendUpdater.h>

namespace QApt { class Backend; }

class ApplicationBackend;
class ApplicationUpdates : public AbstractBackendUpdater
{
    Q_OBJECT
    public:
        explicit ApplicationUpdates(ApplicationBackend* parent);
        
        virtual bool hasUpdates() const;
        virtual qreal progress() const;
        virtual void start();
        void setBackend(QApt::Backend* b);
    public slots:
        void progress(const QString& message, int percentage);
        void downloadMessage(int flag, const QString& message);
        void installMessage(const QString& message);

    private:
        QApt::Backend* m_aptBackend;
        ApplicationBackend* m_appBackend;
        qreal m_progress;
};

#endif // APPLICATIONUPDATES_H
