/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "DiscoverNotifier.h"
#include <KStatusNotifierItem>
#include <QObject>

class NotifierItem : public QObject
{
    Q_OBJECT
public:
    NotifierItem(const std::chrono::seconds &checkDelay);

    void setupNotifierItem();
    void refresh();

    bool isStatusNotifierEnabled() const
    {
        return m_statusNotifierEnabled;
    }
    void setStatusNotifierEnabled(bool enabled);

private:
    // Whether the Status Notifier Item is enabled; if disabled it will never be shown
    bool m_statusNotifierEnabled = false;
    void refreshStatusNotifierVisibility();
    void setStatusNotifierVisibility(bool visible);
    bool shouldShowStatusNotifier() const;
    DiscoverNotifier m_notifier;
    QPointer<KStatusNotifierItem> m_item;
};
