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

#include "AvailableView.h"

#include <QtCore/QFile>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>
#include <QtXml/QDomDocument>

#include <KCategorizedSortFilterProxyModel>
#include <KIcon>
#include <KLocale>
#include <KSeparator>
#include <KStandardDirs>
#include <KDebug>

#include <LibQApt/Backend>

#include "AbstractViewBase.h"
#include "ApplicationBackend.h"
#include "BreadcrumbWidget/BreadcrumbItem.h"
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "CategoryView/Category.h"
#include "CategoryView/CategoryViewWidget.h"

bool categoryLessThan(Category *c1, const Category *c2)
{
    return (QString::localeAwareCompare(c1->name(), c2->name()) < 0);
}

AvailableView::AvailableView(QWidget *parent, ApplicationBackend *appBackend)
        : QWidget(parent)
        , m_backend(0)
        , m_appBackend(appBackend)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    m_viewStack = new QStackedWidget(this);

    m_categoryViewWidget = new CategoryViewWidget(m_viewStack);

    m_breadcrumbWidget = new BreadcrumbWidget(this);

    KSeparator *horizonatalSeparator = new KSeparator(this);
    horizonatalSeparator->setOrientation(Qt::Horizontal);

    populateCategories();

    QString rootName = i18n("Get Software");
    KIcon rootIcon = KIcon("applications-other");
    m_categoryViewWidget->setCategories(m_categoryList, rootName, rootIcon);
    m_breadcrumbWidget->setRootItem(m_categoryViewWidget->breadcrumbItem());

    m_viewStack->addWidget(m_categoryViewWidget);
    m_viewStack->setCurrentWidget(m_categoryViewWidget);

    layout->addWidget(m_breadcrumbWidget);
    layout->addWidget(horizonatalSeparator);
    layout->addWidget(m_viewStack);

    connect(m_breadcrumbWidget, SIGNAL(itemActivated(BreadcrumbItem *)),
            this, SLOT(activateBreadcrumbItem(BreadcrumbItem *)));
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
}

void AvailableView::populateCategories()
{
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
        Category *category = new Category(this, node);
        m_categoryList << category;

        node = node.nextSibling();
    }

    qSort(m_categoryList.begin(), m_categoryList.end(), categoryLessThan);
}

void AvailableView::registerNewSubView(AbstractViewBase *subView)
{
    AbstractViewBase *currentView = static_cast<AbstractViewBase *>(m_viewStack->currentWidget());
    BreadcrumbItem *currentItem = m_breadcrumbWidget->breadcrumbForView(currentView);

    // If we are activating a new subView from a view that already has
    // children, the old ones must go
    if (currentItem->hasChildren()) {
        m_breadcrumbWidget->removeItem(currentItem->childItem());
    }

    m_viewStack->addWidget(subView);
    m_viewStack->setCurrentWidget(subView);
    m_breadcrumbWidget->addLevel(subView->breadcrumbItem());
}

void AvailableView::switchToSubView(AbstractViewBase *subView)
{
    m_viewStack->setCurrentWidget(subView);
    m_breadcrumbWidget->setItemBolded(m_breadcrumbWidget->breadcrumbForView(subView));
}

void AvailableView::activateBreadcrumbItem(BreadcrumbItem *item)
{
    AbstractViewBase *toActivate = item->associatedView();
    if (!toActivate) {
        // Screwed
        return;
    }

    m_viewStack->setCurrentWidget(toActivate);
    m_breadcrumbWidget->setItemBolded(item);
}

// void AvailableView::showAppDetails(Application *app)
// {
//     BreadcrumbItem *parent = m_breadcrumbWidget->breadcrumbForView(m_appView);
// 
//     if (parent) {
//         if (parent->hasChildren()) {
//             m_breadcrumbWidget->removeItem(parent->childItem());
//         }
//     }
// 
//     m_appDetailsWidget = new ApplicationDetailsWidget(this, app);
// 
//     BreadcrumbItem *item = new BreadcrumbItem(m_breadcrumbWidget);
//     item->setText(app->name());
//     item->setIcon(KIcon(app->icon()));
//     item->setAssociatedWidget(m_appDetailsWidget);
//     m_breadcrumbWidget->addLevel(item);
// 
//     parent->setChildItem(item);
//     m_viewStack->addWidget(m_appDetailsWidget);
//     m_viewStack->setCurrentWidget(m_appDetailsWidget);
// }

#include "AvailableView.moc"
