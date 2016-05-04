/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
#ifndef PACKAGEKITNOTIFIER_H
#define PACKAGEKITNOTIFIER_H

#include <BackendNotifierModule.h>
#include <QVariantList>
#include <PackageKit/Transaction>

class QTimer;

class PackageKitNotifier : public BackendNotifierModule
{
Q_OBJECT
Q_PLUGIN_METADATA(IID "org.kde.discover.BackendNotifierModule")
Q_INTERFACES(BackendNotifierModule)
public:
    enum Update {
        NoUpdate,
        Security,
        Normal
    };
    Q_ENUMS(Update)
    explicit PackageKitNotifier(QObject* parent = nullptr);
    ~PackageKitNotifier() override;

    bool isSystemUpToDate() const final;
    uint securityUpdatesCount() final;
    uint updatesCount() final;
    void recheckSystemUpdateNeeded() final;

private Q_SLOTS:
    void package(PackageKit::Transaction::Info info, const QString &packageID, const QString &summary);
    void finished(PackageKit::Transaction::Exit exit, uint);
    
private:
    Update m_update;
    uint m_securityUpdates;
    uint m_normalUpdates;
};

#endif
