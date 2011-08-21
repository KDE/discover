#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QtCore/QAbstractItemModel>

class UpdateItem;

class UpdateModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit UpdateModel(QObject *parent = 0);
    ~UpdateModel();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    UpdateItem *itemFromIndex(const QModelIndex &index) const;

    void addItem(UpdateItem *item);

private:
    UpdateItem *m_rootItem;
};

#endif // UPDATEMODEL_H
