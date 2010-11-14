/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "BreadcrumbItem.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QPushButton>

#include "../AbstractViewBase.h"
#include "BreadcrumbItemButton.h"

BreadcrumbItem::BreadcrumbItem(QWidget *parent)
    : KHBox(parent)
    , m_hasChildren(false)
{
    m_button = new BreadcrumbItemButton(this);
    hide();

    connect(m_button, SIGNAL(clicked()), this, SLOT(emitActivated()));
}

BreadcrumbItem::~BreadcrumbItem()
{
}

BreadcrumbItem *BreadcrumbItem::childItem() const
{
    return m_childItem;
}

AbstractViewBase *BreadcrumbItem::associatedView() const
{
    return m_associatedView;
}

bool BreadcrumbItem::hasChildren() const
{
    return m_hasChildren;
}

void BreadcrumbItem::setChildItem(BreadcrumbItem *child)
{
    m_childItem = child;
    // Setting a null pointer would technically make this false...
    m_hasChildren = true;

    if (!m_button->text().contains(QString::fromUtf8("➜"))) {
        m_button->setText(m_button->text() % ' ' % QString::fromUtf8("➜"));
    }
}

void BreadcrumbItem::setAssociatedView(AbstractViewBase *view)
{
    m_associatedView = view;
}

void BreadcrumbItem::setText(const QString &text)
{
    m_button->setText(text);
}

void BreadcrumbItem::setIcon(const QIcon &icon)
{
    m_button->setIcon(icon);
}

void BreadcrumbItem::setActive(bool active)
{
    m_button->setActive(active);
}

void BreadcrumbItem::emitActivated()
{
    emit activated(this);
}

#include "BreadcrumbItem.moc"
