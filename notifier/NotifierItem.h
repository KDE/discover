/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NOTIFIERITEM_H
#define NOTIFIERITEM_H

#include "DiscoverNotifier.h"
#include <KStatusNotifierItem>
#include <QObject>

class NotifierItem : public QObject
{
    Q_OBJECT
public:
    NotifierItem();

    void setupNotifierItem();
    void refresh();

    bool isVisible() const
    {
        return m_visible;
    }
    void setVisible(bool visible);

private:
    bool m_visible = false;
    DiscoverNotifier m_notifier;
    QPointer<KStatusNotifierItem> m_item;
};

#endif // NOTIFIERITEM_H
