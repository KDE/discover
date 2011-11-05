#include "FilterModel.h"

FilterModel::FilterModel(QObject *parent) :
    QStandardItemModel(parent)
{
}

void FilterModel::reload()
{
    clear();
    populate();
}
