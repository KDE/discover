/*
 *   Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DISCOVERACTION_H
#define DISCOVERACTION_H

#include <QAction>

class KXmlGuiWindow;
class DiscoverAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY proxyChanged)
    Q_PROPERTY(KXmlGuiWindow* mainWindow READ mainWindow WRITE setMainWindow NOTIFY proxyChanged)
    Q_PROPERTY(QAction::Priority priority READ priority WRITE setPriority NOTIFY proxyChanged)
    Q_PROPERTY(QString shortcut READ stringShortcut WRITE setShortcutString NOTIFY proxyChanged)
    public:
        explicit DiscoverAction(QObject* parent = nullptr);
        
        QString iconName() const;
        void setIconName(const QString& name);
        
        KXmlGuiWindow* mainWindow() const;
        void setMainWindow(KXmlGuiWindow* w);
        
        QString stringShortcut() const;
        void setShortcutString(const QString& name);

    Q_SIGNALS:
        void proxyChanged();
};

#endif // DISCOVERACTION_H
