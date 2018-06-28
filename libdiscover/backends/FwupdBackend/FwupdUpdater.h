/***************************************************************************
*   Copyright © 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>      *
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

#ifndef FWUPDUPDATER_H
#define FWUPDUPDATER_H

#include <resources/AbstractBackendUpdater.h>
#include <resources/AbstractResource.h>
#include "FwupdBackend.h"
#include "FwupdTransaction.h"

class FwupdUpdater : public AbstractBackendUpdater
{
Q_OBJECT
public:
    explicit FwupdUpdater(FwupdBackend * parent = nullptr);
    ~FwupdUpdater() override;

    void prepare() override;
    int updatesCount();

    quint64 downloadSpeed() const override;
    double updateSize() const override;
    bool isMarked(AbstractResource* res) const override;
    bool isProgressing() const override;
    bool isCancelable() const override;
    QDateTime lastUpdate() const override;
    QList<AbstractResource*> toUpdate() const override;
    void addResources(const QList<AbstractResource*>& apps) override;
    void removeResources(const QList<AbstractResource*>& apps) override;
    qreal progress() const override;
    bool hasUpdates() const override;
    void setProgressing(bool progressing);

    QSet<QString> involvedResources(const QSet<AbstractResource*>& resources) const;
    QSet<AbstractResource*> resourcesForResourceId(const QSet<QString>& resids) const;

public Q_SLOTS:

        void start() override;
        void cancel() override;

Q_SIGNALS:
    void updatesCountChanged();

private:
    FwupdTransaction* m_transaction;
    FwupdBackend * const m_backend;
    bool m_isCancelable;
    bool m_isProgressing;
    int m_percentage;
    QDateTime m_lastUpdate;

    QSet<AbstractResource*> m_toUpgrade;
    QSet<AbstractResource*> m_allUpgradeable;
};

#endif
