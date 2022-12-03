/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSTransaction.h"
#include "KNSBackend.h"
#include <KLocalizedString>
#include <KNSCore/Engine>
#include <KNSCore/Question>

#include <QTimer>
#include <Transaction/TransactionModel.h>

KNSTransaction::KNSTransaction(QObject *parent, KNSResource *res, Role role)

    : Transaction(parent, res, role)
    , m_id(res->entry().uniqueId())
{
    setCancellable(false);

    auto manager = res->knsBackend()->engine();
    connect(manager, &KNSCore::Engine::signalEntryEvent, this, [this](const KNSCore::EntryInternal &entry, KNSCore::EntryInternal::EntryEvent event) {
        switch (event) {
        case KNSCore::EntryInternal::StatusChangedEvent:
            anEntryChanged(entry);
            break;
        case KNSCore::EntryInternal::DetailsLoadedEvent:
        case KNSCore::EntryInternal::AdoptedEvent:
        case KNSCore::EntryInternal::UnknownEvent:
        default:
            break;
        }
    });
    TransactionModel::global()->addTransaction(this);

    std::function<void()> actionFunction;
    auto engine = res->knsBackend()->engine();
    if (role == RemoveRole)
        actionFunction = [res, engine]() {
            engine->uninstall(res->entry());
        };
    else if (res->entry().status() == KNS3::Entry::Updateable)
        actionFunction = [res, engine]() {
            engine->install(res->entry(), -1);
        };
    else if (res->linkIds().isEmpty())
        actionFunction = [res]() {
            qWarning() << "No installable candidates in the KNewStuff entry" << res->entry().name() << "with id" << res->entry().uniqueId() << "on the backend"
                       << res->backend()->name()
                       << "There should always be at least one downloadable item in an OCS entry, and if there isn't, we should consider it broken. OCS "
                          "can technically show them, but if there is nothing to install, it cannot be installed.";
        };
    else
        actionFunction = [res, engine]() {
            engine->install(res->entry());
        };
    QTimer::singleShot(0, res, actionFunction);
}

void KNSTransaction::addQuestion(KNSCore::Question *question)
{
    Q_ASSERT(question->questionType() == KNSCore::Question::ContinueCancelQuestion);
    m_questions << question;

    Q_EMIT proceedRequest(question->title(), question->question());
}

void KNSTransaction::anEntryChanged(const KNSCore::EntryInternal &entry)
{
    if (entry.uniqueId() == m_id) {
        switch (entry.status()) {
        case KNS3::Entry::Invalid:
            qWarning() << "invalid status for" << entry.uniqueId() << entry.status();
            break;
        case KNS3::Entry::Installing:
        case KNS3::Entry::Updating:
            setStatus(CommittingStatus);
            break;
        case KNS3::Entry::Downloadable:
        case KNS3::Entry::Installed:
        case KNS3::Entry::Deleted:
        case KNS3::Entry::Updateable:
            if (status() != DoneStatus) {
                setStatus(DoneStatus);
            }
            break;
        }
    }
}

void KNSTransaction::cancel()
{
    for (auto q : m_questions) {
        q->setResponse(KNSCore::Question::CancelResponse);
    }
    setStatus(CancelledStatus);
}

void KNSTransaction::proceed()
{
    m_questions.takeFirst()->setResponse(KNSCore::Question::ContinueResponse);
}
