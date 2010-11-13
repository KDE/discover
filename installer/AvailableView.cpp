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

#include <QStandardItemModel>
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

#include "Application.h"
#include "ApplicationWidget.h"
#include "ApplicationBackend.h"
#include "ApplicationModel/ApplicationView.h"
#include "BreadcrumbWidget/BreadcrumbItem.h"
#include "BreadcrumbWidget/BreadcrumbWidget.h"
#include "CategoryView/Category.h"
#include "CategoryView/CategoryView.h"

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

    m_categoryView = new CategoryView(m_viewStack);

    m_breadcrumbWidget = new BreadcrumbWidget(this);
    BreadcrumbItem *rootItem = new BreadcrumbItem(m_breadcrumbWidget);
    rootItem->setText(i18n("Get Software"));
    rootItem->setIcon(KIcon("applications-other"));
    rootItem->setAssociatedWidget(m_categoryView);
    m_breadcrumbWidget->setRootItem(rootItem);
    connect(m_breadcrumbWidget, SIGNAL(itemActivated(BreadcrumbItem *)),
            this, SLOT(activateItem(BreadcrumbItem *)));

    KSeparator *horizonatalSeparator = new KSeparator(this);
    horizonatalSeparator->setOrientation(Qt::Horizontal);

    populateCategories();

    m_categoryModel = new QStandardItemModel(this);

    foreach (Category *category, m_categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setData(i18n("Categories"), KCategorizedSortFilterProxyModel::CategoryDisplayRole);

        if (category->hasSubCategories()) {
            categoryItem->setData(SubCatType, CategoryTypeRole);
        } else {
            categoryItem->setData(CategoryType, CategoryTypeRole);
        }

        m_categoryModel->appendRow(categoryItem);
    }

    KCategorizedSortFilterProxyModel *proxy = new KCategorizedSortFilterProxyModel(this);
    proxy->setSourceModel(m_categoryModel);
    proxy->setCategorizedModel(true);
    proxy->sort(0);
    m_categoryView->setModel(proxy);

    m_viewStack->addWidget(m_categoryView);
    m_viewStack->setCurrentWidget(m_categoryView);

    layout->addWidget(m_breadcrumbWidget);
    layout->addWidget(horizonatalSeparator);
    layout->addWidget(m_viewStack);

    connect(m_categoryView, SIGNAL(activated(const QModelIndex &)),
           this, SLOT(changeView(const QModelIndex &)));
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

void AvailableView::changeView(const QModelIndex &index)
{
    QWidget *view = m_viewHash.value(index);

    BreadcrumbItem *breadcrumbItem = m_breadcrumbWidget->breadcrumbForWidget(m_viewStack->currentWidget());
    if (breadcrumbItem->hasChildren()) {
        m_breadcrumbWidget->removeItem(breadcrumbItem->childItem());
    }

    if (!view) {
        switch (index.data(CategoryTypeRole).toInt()) {
        case CategoryType: {
            view = new ApplicationView(this, m_appBackend);
            m_appView = static_cast<ApplicationView *>(view);
            connect(m_appView, SIGNAL(destroyed(QObject *)), this, SLOT(onViewDestroyed(QObject *)));
            m_viewStack->addWidget(view);
            m_appView->setBackend(m_backend);
            Category *category = m_categoryList.at(index.row());

            BreadcrumbItem *item = new BreadcrumbItem(m_breadcrumbWidget);
            item->setText(category->name());
            item->setIcon(KIcon(category->icon()));
            item->setAssociatedWidget(m_appView);
            m_breadcrumbWidget->addLevel(item);

            m_appView->setAndOrFilters(category->andOrFilters());
            m_appView->setNotFilters(category->notFilters());

            connect(m_appView, SIGNAL(infoButtonClicked(Application *)),
                    this, SLOT(showAppDetails(Application *)));
        }
            break;
        case SubCatType:
            break;
        default:
            break;
        }
    }

    m_viewStack->setCurrentWidget(view);

    m_viewHash[index] = view;
}

void AvailableView::activateItem(BreadcrumbItem *item)
{
    QWidget *toActivate = item->associatedWidget();
    if (!toActivate) {
        // Screwed
        return;
    }

    m_viewStack->setCurrentWidget(toActivate);
}

void AvailableView::showAppDetails(Application *app)
{
    BreadcrumbItem *parent = m_breadcrumbWidget->breadcrumbForWidget(m_appView);

    if (parent) {
        if (parent->hasChildren()) {
            m_breadcrumbWidget->removeItem(parent->childItem());
        }
    }

    m_appWidget = new ApplicationWidget(this, app);
    // FIXME: This leaks if we go back and choose another app
    // Keep only one ApplicationWidget at a time as a member variable
    // and only change it on showAppDetails

    BreadcrumbItem *item = new BreadcrumbItem(m_breadcrumbWidget);
    item->setText(app->name());
    item->setIcon(KIcon(app->icon()));
    item->setAssociatedWidget(m_appWidget);
    m_breadcrumbWidget->addLevel(item);

    parent->setChildItem(item);
    m_viewStack->addWidget(m_appWidget);
    m_viewStack->setCurrentWidget(m_appWidget);
}

void AvailableView::onViewDestroyed(QObject *object)
{
    QWidget *view = static_cast<QWidget *>(object);
    m_viewHash.remove(m_viewHash.key(view));
}

#include "AvailableView.moc"
