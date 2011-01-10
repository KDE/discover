/***************************************************************************
 *   Copyright © 2010-2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "AvailableView.h"

#include <QtCore/QFile>
#include <QtGui/QStackedWidget>
#include <QtXml/QDomDocument>

#include <KCategorizedSortFilterProxyModel>
#include <KIcon>
#include <KLocale>
#include <KStandardDirs>

#include <LibQApt/Backend>

#include "ApplicationBackend.h"
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "CategoryView/Category.h"
#include "CategoryView/CategoryViewWidget.h"

bool categoryLessThan(Category *c1, const Category *c2)
{
    return (QString::localeAwareCompare(c1->name(), c2->name()) < 0);
}

AvailableView::AvailableView(QWidget *parent, ApplicationBackend *appBackend)
        : AbstractViewContainer(parent)
        , m_backend(0)
        , m_appBackend(appBackend)
{

    m_categoryViewWidget = new CategoryViewWidget(m_viewStack, m_appBackend);
    populateCategories();

    QString rootName = i18n("Get Software");
    KIcon rootIcon = KIcon("applications-other");
    m_categoryViewWidget->setCategories(m_categoryList, rootName, rootIcon);
    m_breadcrumbWidget->setRootItem(m_categoryViewWidget->breadcrumbItem());

    m_viewStack->addWidget(m_categoryViewWidget);
    m_viewStack->setCurrentWidget(m_categoryViewWidget);

    connect(m_appBackend, SIGNAL(xapianReloaded()),
            m_breadcrumbWidget, SLOT(startSearch()));
    connect(m_categoryViewWidget, SIGNAL(registerNewSubView(AbstractViewBase *)),
            this, SLOT(registerNewSubView(AbstractViewBase *)));
    connect(m_categoryViewWidget, SIGNAL(switchToSubView(AbstractViewBase *)),
            this, SLOT(switchToSubView(AbstractViewBase *)));
}

AvailableView::~AvailableView()
{
}

void AvailableView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;

    m_categoryViewWidget->setBackend(backend);
}

void AvailableView::populateCategories()
{
    qDeleteAll(m_categoryList);
    m_categoryList.clear();
    QFile menuFile(KStandardDirs::locate("appdata", "categories.xml"));

    if (!menuFile.open(QIODevice::ReadOnly)) {
        // Broken install or broken FS
        return;
    }

    QDomDocument menuDocument;
    QString error;
    int line;
    menuDocument.setContent(&menuFile, &error, &line);

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while(!node.isNull())
    {
        m_categoryList << new Category(node);

        node = node.nextSibling();
    }

    qSort(m_categoryList.begin(), m_categoryList.end(), categoryLessThan);
}

#include "AvailableView.moc"
