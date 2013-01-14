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

DiscoverAction::DiscoverAction(QObject* parent)
    : KAction(parent)
{}

void DiscoverAction::setIconName(const QString& name)
{
    setIcon(KIcon(name));
}

QString DiscoverAction::iconName() const
{
    return icon().themeName();
}

KXmlGuiWindow* DiscoverAction::mainWindow() const
{
    return qobject_cast<KXmlGuiWindow*>(KAction::parentWidget());
}

void DiscoverAction::setMainWindow(KXmlGuiWindow* w)
{
    w->actionCollection()->addAction(objectName(), this);
}
