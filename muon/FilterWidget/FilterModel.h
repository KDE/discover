#ifndef FILTERMODEL_H
#define FILTERMODEL_H

#include <QStandardItemModel>

class FilterModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit FilterModel(QObject *parent = 0);
    
    virtual void populate() =0;
    void reload();
};

#endif // FILTERMODEL_H
