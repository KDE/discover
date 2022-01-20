/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKREFRESHAPPSTREAMMETADATAJOB_H
#define FLATPAKREFRESHAPPSTREAMMETADATAJOB_H

#include "flatpak-helper.h"
#include <QThread>

class FlatpakRefreshAppstreamMetadataJob : public QThread
{
    Q_OBJECT
public:
    FlatpakRefreshAppstreamMetadataJob(FlatpakInstallation *installation, FlatpakRemote *remote);
    ~FlatpakRefreshAppstreamMetadataJob() override;

    void cancel();
    void run() override;

Q_SIGNALS:
    void jobRefreshAppstreamMetadataFinished(FlatpakInstallation *installation, FlatpakRemote *remote);

private:
    GCancellable *m_cancellable;
    FlatpakInstallation *m_installation;
    FlatpakRemote *m_remote;
};

#endif
