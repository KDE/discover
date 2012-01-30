#include "CategoryModel.h"
#include "CategoryView/Category.h"
#include <KIcon>
#include <KCategorizedSortFilterProxyModel>

CategoryModel::CategoryModel(QObject* parent)
    : QStandardItemModel(parent)
{}

void CategoryModel::setCategories(const QList<Category *> &categoryList,
                                  const QString &rootName)
{
    m_categoryList = categoryList;
    foreach (Category *category, m_categoryList) {
        QStandardItem *categoryItem = new QStandardItem;
        categoryItem->setText(category->name());
        categoryItem->setIcon(KIcon(category->icon()));
        categoryItem->setEditable(false);
        categoryItem->setData(rootName, KCategorizedSortFilterProxyModel::CategoryDisplayRole);

        if (category->hasSubCategories()) {
            categoryItem->setData(SubCatType, CategoryTypeRole);
        } else {
            categoryItem->setData(CategoryType, CategoryTypeRole);
        }

        appendRow(categoryItem);
    }
}

Category* CategoryModel::categoryForIndex(const QModelIndex& index)
{
    return m_categoryList.at(index.row());
}
