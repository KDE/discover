/***************************************************************************
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#ifndef FLATPAKFETCHDATAJOB_H
#define FLATPAKFETCHDATAJOB_H

extern "C" {
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

#include <QThread>

class FlatpakResource;
class FlatpakFetchDataJob : public QThread
{
    Q_OBJECT
public:
    enum DataKind {
        FetchMetadata = 0,
        FetchSize = 1,
    };

    FlatpakFetchDataJob(FlatpakInstallation *installation, FlatpakResource *app, DataKind kind);
    ~FlatpakFetchDataJob();

    void cancel();
    void run() override;

Q_SIGNALS:
    void jobFetchMetadataFinished(FlatpakInstallation *installation, FlatpakResource *resource, const QByteArray &metadata);
    void jobFetchSizeFinished(FlatpakResource *resource, int downloadSize, int installedSize);

private:
    FlatpakRef * createFakeRef(FlatpakResource *resource);

    GCancellable *m_cancellable;
    FlatpakResource *m_app;
    FlatpakInstallation *m_installation;
    DataKind m_kind;
};

#endif // FLATPAKFETCHDATAJOB_H


