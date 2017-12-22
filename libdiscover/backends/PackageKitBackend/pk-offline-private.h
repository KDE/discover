/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PK_OFFLINE_PRIVATE_H
#define __PK_OFFLINE_PRIVATE_H

//NOTE: please don't modify, comes from upstream PackageKit/lib/packagekit-glib2/pk-offline-private.h

#ifndef PK_OFFLINE_DESTDIR
#define PK_OFFLINE_DESTDIR		""
#endif

/* the state file for regular offline update */
#define PK_OFFLINE_PREPARED_FILENAME	PK_OFFLINE_DESTDIR "/var/lib/PackageKit/prepared-update"
/* the state file for offline system upgrade */
#define PK_OFFLINE_PREPARED_UPGRADE_FILENAME \
					PK_OFFLINE_DESTDIR "/var/lib/PackageKit/prepared-upgrade"

/* the trigger file that systemd uses to start a different boot target */
#define PK_OFFLINE_TRIGGER_FILENAME	PK_OFFLINE_DESTDIR "/system-update"

/* the keyfile describing the outcome of the latest offline update */
#define PK_OFFLINE_RESULTS_FILENAME	PK_OFFLINE_DESTDIR "/var/lib/PackageKit/offline-update-competed"

/* the action to take when the offline update has completed, e.g. restart */
#define PK_OFFLINE_ACTION_FILENAME	PK_OFFLINE_DESTDIR "/var/lib/PackageKit/offline-update-action"

/* the group name for the offline updates results keyfile */
#define PK_OFFLINE_RESULTS_GROUP	"PackageKit Offline Update Results"

#endif
