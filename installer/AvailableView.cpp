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
#include <QtXml/QDomDocument>

#include <KCategorizedSortFilterProxyModel>
#include <KIcon>
#include <KLocale>
#include <KStandardDirs>
#include <KDebug>

#include <LibQApt/Backend>

#include "ApplicationBackend.h"
#include "ApplicationModel/ApplicationView.h"
#include "CategoryView/Category.h"
#include "CategoryView/CategoryView.h"

AvailableView::AvailableView(QWidget *parent, ApplicationBackend *appBackend)
        : QStackedWidget(parent)
        , m_backend(0)
        , m_appBackend(appBackend)
{
    m_categoryView = new CategoryView(this);

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

    addWidget(m_categoryView);

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
}

void AvailableView::changeView(const QModelIndex &index)
{
    QWidget *view = m_viewHash.value(index);

    if (!view) {
        kDebug() << index.data(CategoryTypeRole).toInt();
        switch (index.data(CategoryTypeRole).toInt()) {
        case CategoryType: {
            view = new ApplicationView(this, m_appBackend);
            ApplicationView *appView = static_cast<ApplicationView *>(view);
            addWidget(view);
            appView->setBackend(m_backend);
            Category *category = m_categoryList.at(index.row());

            kDebug() << category->andOrFilters();
            appView->setAndOrFilters(category->andOrFilters());
            appView->setNotFilters(category->notFilters());
        }
            break;
        case SubCatType:
            break;
        default:
            break;
        }
    }

    setCurrentWidget(view);

    m_viewHash[index] = view;
}

#include "AvailableView.moc"
