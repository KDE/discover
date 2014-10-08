/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "DummyTransaction.h"
#include "DummyBackend.h"
#include "DummyResource.h"
#include <Transaction/TransactionModel.h>
#include <QTimer>
#include <QDebug>
#include <KRandom>

DummyTransaction::DummyTransaction(DummyResource* app, Role action)
    : Transaction(app->backend(), app, action)
    , m_app(app)
{
    iterateTransaction();
}

DummyTransaction::DummyTransaction(DummyResource* app, const AddonList& addons, Transaction::Role role)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
{
    iterateTransaction();
}

void DummyTransaction::iterateTransaction()
{
    if(progress()<100) {
        setProgress(progress()+10);
        QTimer::singleShot(/*KRandom::random()%*/20, this, SLOT(iterateTransaction()));
    } else
        finishTransaction();
}

void DummyTransaction::finishTransaction()
{
    AbstractResource::State newState;
    switch(role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    m_app->setAddons(addons());
    qDebug() << "done...";
    TransactionModel::global()->removeTransaction(this);
    deleteLater();
}
