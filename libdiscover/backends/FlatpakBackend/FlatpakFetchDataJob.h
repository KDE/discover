/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FLATPAKFETCHDATAJOB_H
#define FLATPAKFETCHDATAJOB_H

#include <QByteArray>
extern "C" {
#include <flatpak.h>
}
#include <glib.h>

class FlatpakResource;

namespace FlatpakRunnables
{
    FlatpakRemoteRef* findRemoteRef(FlatpakResource *app, GCancellable* cancellable);

    QByteArray fetchMetadata(FlatpakResource *app, GCancellable* cancellable);
}

#endif // FLATPAKFETCHDATAJOB_H


