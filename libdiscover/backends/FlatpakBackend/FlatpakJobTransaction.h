/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "flatpak-helper.h"
#include <QPointer>
#include <Transaction/Transaction.h>

#include <gio/gio.h>
#include <glib.h>

#include "FlatpakTransactionThread.h"

class FlatpakResource;
class FlatpakJobTransaction : public Transaction
{
    Q_OBJECT
public:
    FlatpakJobTransaction(FlatpakResource *app, Role role);
    ~FlatpakJobTransaction();

    Q_SCRIPTABLE void proceed() override;
    void cancel() override;

    /** Mapping of repositories where a key is an installation path and a value is a list of names */
    using Repositories = QMap<QString, QStringList>;

public Q_SLOTS:
    void finishTransaction(bool cancelled, const QString &errorMessage, const FlatpakTransactionThread::Repositories &addedRepositories, bool success);

Q_SIGNALS:
    void repositoriesAdded(const Repositories &repositories);

public:
    QPointer<FlatpakResource> m_app;
    QPointer<FlatpakTransactionThread> m_thread;
};
