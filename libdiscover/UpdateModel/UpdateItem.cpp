/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>

#include <QStringBuilder>
#include <KLocalizedString>
#include "libdiscover_debug.h"

UpdateItem::UpdateItem(AbstractResource *app)
    : m_app(app)
{
}

UpdateItem::~UpdateItem()
{
}

AbstractResource *UpdateItem::app() const
{
    return m_app;
}

QString UpdateItem::name() const
{
    return m_app->name();
}

QVariant UpdateItem::icon() const
{
    return m_app->icon();
}

qint64 UpdateItem::size() const
{
    return m_app->size();
}

static bool isMarked(AbstractResource* res)
{
    return res->backend()->backendUpdater()->isMarked(res);
}

Qt::CheckState UpdateItem::checked() const
{
    return isMarked(app()) ? Qt::Checked : Qt::Unchecked;
}

qreal UpdateItem::progress() const
{
    return m_progress;
}

void UpdateItem::setProgress(qreal progress)
{
    m_progress = progress;
}

QString UpdateItem::changelog() const
{
    return m_changelog;
}

void UpdateItem::setChangelog(const QString& changelog)
{
    m_changelog = changelog;
}
