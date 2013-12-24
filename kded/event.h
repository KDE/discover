/***************************************************************************
 *   Copyright © 2009 Harald Sitter <apachelogger@ubuntu.com>              *
 *   Copyright © 2009 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef EVENT_H
#define EVENT_H

#include <QtCore/QObject>

// #include <KDebug>
#include <KLocalizedString>

// for implementing classes

class KStatusNotifierItem;

#define NOTIFICATION_ICON_SIZE 22,22

class Event
            : public QObject
{
    Q_OBJECT
public:
    Event(QObject* parent, const QString &name);

    virtual ~Event();

public slots:
    bool isHidden() const;
    void show(const QString &icon, const QString &text, const QStringList &actions,
              const QString &tTipIcon = QString(), const QString &actionIcon = QString());
    void ignore();
    void update(const QString &icon, const QString &text, const QString &tTipIcon = QString());
    void run();
    virtual void reloadConfig();

private slots:
    bool readHiddenConfig();
    void writeHiddenConfig(bool value);
    void readNotifyConfig();
    void hide();
    void notifyClosed();

private:
    QString m_hiddenCfgString;
    const QString m_name;
    bool m_hidden;
    bool m_useKNotify;
    bool m_useTrayIcon;

    KStatusNotifierItem *m_notifierItem;

protected:
    bool m_active;
    bool m_verbose;
};

#endif
