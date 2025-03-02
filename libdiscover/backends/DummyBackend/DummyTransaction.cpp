/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DummyTransaction.h"
#include "DummyBackend.h"
#include "DummyResource.h"
#include <KRandom>
#include <QDebug>
#include <QTimer>

// #define TEST_PROCEED

DummyTransaction::DummyTransaction(DummyResource *app, Role role)
    : DummyTransaction(app, {}, role)
{
}

DummyTransaction::DummyTransaction(DummyResource *app, const AddonList &addons, Transaction::Role role)
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

    if (progress() < 100) {
        setProgress(qBound(0, progress() + QRandomGenerator::global()->bounded(5), 100));
        QTimer::singleShot(/*KRandom::random()%*/ 10, this, &DummyTransaction::iterateTransaction);
    } else if (status() == DownloadingStatus) {
        setStatus(CommittingStatus);
        QTimer::singleShot(/*KRandom::random()%*/ 10, this, &DummyTransaction::iterateTransaction);
#ifdef TEST_PROCEED
    } else if (resource()->name() == "Dummy 101") {
        Q_EMIT proceedRequest(QStringLiteral("yadda yadda"),
                              QStringLiteral("Biii BOooo<ul><li>A</li><li>A</li>") + QStringLiteral("<li>A</li>").repeated(2)
                                  + QStringLiteral("<li>A</li></ul>"));
#endif
    } else {
        finishTransaction();
    }
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
    AbstractResource::State newState = AbstractResource::State::Broken;
    switch (role()) {
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

#include "moc_DummyTransaction.cpp"
