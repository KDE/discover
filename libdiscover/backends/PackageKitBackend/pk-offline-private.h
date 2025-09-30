/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * SPDX-FileCopyrightText: 2014 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

// NOTE: please don't modify, comes from upstream PackageKit/lib/packagekit-glib2/pk-offline-private.h

#ifndef PK_OFFLINE_DESTDIR
#define PK_OFFLINE_DESTDIR ""
#endif

/* the state file for regular offline update */
#define PK_OFFLINE_PREPARED_FILENAME PK_OFFLINE_DESTDIR "/var/lib/PackageKit/prepared-update"
/* the state file for offline system upgrade */
#define PK_OFFLINE_PREPARED_UPGRADE_FILENAME PK_OFFLINE_DESTDIR "/var/lib/PackageKit/prepared-upgrade"

/* the trigger file that systemd uses to start a different boot target */
#define PK_OFFLINE_TRIGGER_FILENAME PK_OFFLINE_DESTDIR "/system-update"

/* the keyfile describing the outcome of the latest offline update */
#define PK_OFFLINE_RESULTS_FILENAME PK_OFFLINE_DESTDIR "/var/lib/PackageKit/offline-update-competed"

/* the action to take when the offline update has completed, e.g. restart */
#define PK_OFFLINE_ACTION_FILENAME PK_OFFLINE_DESTDIR "/var/lib/PackageKit/offline-update-action"

/* the group name for the offline updates results keyfile */
#define PK_OFFLINE_RESULTS_GROUP "PackageKit Offline Update Results"
