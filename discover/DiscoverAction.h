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

#include <KAction>
#include <qdeclarative.h>

class KXmlGuiWindow;
class DiscoverAction : public KAction
{
    Q_OBJECT
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName)
    Q_PROPERTY(KXmlGuiWindow* mainWindow READ mainWindow WRITE setMainWindow)
    Q_PROPERTY(QAction::Priority priority READ priority WRITE setPriority)
    Q_PROPERTY(QString actionsGroup READ actionsGroup WRITE setActionsGroup)
    public:
        explicit DiscoverAction(QObject* parent = 0);
        
        QString iconName() const;
        void setIconName(const QString& name);
        
        KXmlGuiWindow* mainWindow() const;
        void setMainWindow(KXmlGuiWindow* w);
        
        QString actionsGroup() const;
        void setActionsGroup(const QString& name);

    private:
        QActionGroup* m_actionsGroup;
};

#endif // DISCOVERACTION_H
