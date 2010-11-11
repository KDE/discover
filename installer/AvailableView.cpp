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

#include "CategoryModel/Category.h"
#include "CategoryModel/CategoryView.h"

AvailableView::AvailableView(QWidget *parent)
        : QStackedWidget(parent)
{
    m_categoryView = new CategoryView(this);

    QList<Category *> categoryList;

    QFile menuFile(KStandardDirs::locate("appdata", "categories.xml"));
    menuFile.open(QIODevice::ReadOnly);

    QDomDocument menuDocument;
    QString error;
    int line;
    menuDocument.setContent(&menuFile, &error, &line);

    QDomElement root = menuDocument.documentElement();

    QDomNode node = root.firstChild();
    while(!node.isNull())
    {
        Category *category = new Category(this, node);
        categoryList << category;

        node = node.nextSibling();
    }

    QStandardItemModel *categoryModel = new QStandardItemModel(this);

    foreach (Category *category, categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setData(i18n("Categories"), KCategorizedSortFilterProxyModel::CategoryDisplayRole);
        categoryModel->appendRow(categoryItem);
    }

    KCategorizedSortFilterProxyModel *proxy = new KCategorizedSortFilterProxyModel(this);
    proxy->setSourceModel(categoryModel);
    proxy->setCategorizedModel(true);
    proxy->sort(0);
    m_categoryView->setModel(proxy);

    addWidget(m_categoryView);
}

AvailableView::~AvailableView()
{
}

#include "AvailableView.moc"
