/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef MUONNOTIFIERMODULE_H
#define MUONNOTIFIERMODULE_H

#include <KDEDModule>
#include <QVariantList>
#include <BackendNotifierModule.h>

class KStatusNotifierItem;

class Q_DECL_EXPORT MuonNotifierModule : public KDEDModule
{
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.MuonNotifier")
Q_PROPERTY(bool systemUpToDate READ isSystemUpToDate)
Q_PROPERTY(QStringList modules READ loadedModules)
public:
    enum State {
        NoUpdates,
        NormalUpdates,
        SecurityUpdates
    };

    MuonNotifierModule(QObject* parent = 0, const QVariantList& args = QVariantList());
    virtual ~MuonNotifierModule();

    bool isSystemUpToDate() const;

    State state() const;
    QString iconName() const;
    QString message() const;
    QString extendedMessage() const;
    int updatesCount() const;
    int securityUpdatesCount() const;

    void updateStatusNotifier();
    QStringList loadedModules() const;

public Q_SLOTS:
    void configurationChanged();
    void recheckSystemUpdateNeeded();
    void quit();
    void showMuon();

Q_SIGNALS:
    void systemUpdateNeeded();

private:
    void loadBackends();

    KStatusNotifierItem* m_statusNotifier;
    QList<BackendNotifierModule*> m_backends;
    bool m_verbose;
};

#endif //ABSTRACTKDEDMODULE_H
