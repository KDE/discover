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

#include "DiscoverAction.h"
#include <KXmlGuiWindow>
#include <KActionCollection>
#include <QDebug>
#include <QIcon>

DiscoverAction::DiscoverAction(QObject* parent)
    : QAction(parent)
    , m_actionsGroup(nullptr)
{}

void DiscoverAction::setIconName(const QString& name)
{
    setIcon(QIcon::fromTheme(name));
}

QString DiscoverAction::iconName() const
{
    return icon().themeName();
}

KXmlGuiWindow* DiscoverAction::mainWindow() const
{
    return qobject_cast<KXmlGuiWindow*>(QAction::parentWidget());
}

void DiscoverAction::setMainWindow(KXmlGuiWindow* w)
{
    if(m_actionsGroup && !m_actionsGroup->parent())
        m_actionsGroup->setParent(w);
    w->actionCollection()->addAction(objectName(), this);
    w->actionCollection()->setDefaultShortcuts(this, shortcuts());
}

QString DiscoverAction::actionsGroup() const
{
    return m_actionsGroup ? m_actionsGroup->objectName() : QString();
}

void DiscoverAction::setActionsGroup(const QString& name)
{
    static QHash<QString, QActionGroup*> availableGroups;
    QString oldName = actionsGroup();
    if(m_actionsGroup && name != oldName) {
        if(m_actionsGroup->actions().count()<=1) {
            availableGroups.remove(oldName);
            delete m_actionsGroup;
        } else
            m_actionsGroup->removeAction(this);
    }

    if(!name.isEmpty()) {
        m_actionsGroup = availableGroups.value(name);
        if(!m_actionsGroup) {
            m_actionsGroup = new QActionGroup(mainWindow());
            m_actionsGroup->setObjectName(name);
            availableGroups.insert(name, m_actionsGroup);
        }
        m_actionsGroup->addAction(this);
    }
}

void DiscoverAction::setShortcutString(const QString& str)
{
    setShortcut(str);
    if (mainWindow())
        mainWindow()->actionCollection()->setDefaultShortcut(this, str);
}

QString DiscoverAction::stringShortcut() const
{
    return QAction::shortcut().toString();
}
