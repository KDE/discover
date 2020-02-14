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
#include <QTimer>
#include <QDebug>
#include <KRandom>

// #define TEST_PROCEED

DummyTransaction::DummyTransaction(DummyResource* app, Role role)
    : DummyTransaction(app, {}, role)
{
}

DummyTransaction::DummyTransaction(DummyResource* app, const AddonList& addons, Transaction::Role role)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
{
    setCancellable(true);
    setStatus(DownloadingStatus);

    iterateTransaction();
}

void DummyTransaction::iterateTransaction()
{
    if (!m_iterate)
        return;

    if(progress()<100) {
        setProgress(qBound(0, progress()+(KRandom::random()%30), 100));
        QTimer::singleShot(/*KRandom::random()%*/100, this, &DummyTransaction::iterateTransaction);
    } else if (status() == DownloadingStatus) {
        setStatus(CommittingStatus);
        QTimer::singleShot(/*KRandom::random()%*/100, this, &DummyTransaction::iterateTransaction);
    } else
#ifdef TEST_PROCEED
        Q_EMIT proceedRequest(QStringLiteral("yadda yadda"), QStringLiteral("Biii BOooo<ul><li>A</li><li>A</li>") + QStringLiteral("<li>A</li>").repeated(2) + QStringLiteral("<li>A</li></ul>"));
#else
        finishTransaction();
#endif
}

void DummyTransaction::proceed()
{
    finishTransaction();
}

void DummyTransaction::cancel()
{
    m_iterate = false;

    setStatus(CancelledStatus);
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
    m_app->setAddons(addons());
    m_app->setState(newState);
    setStatus(DoneStatus);
    deleteLater();
}
