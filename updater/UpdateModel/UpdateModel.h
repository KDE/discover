#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QtCore/QAbstractItemModel>

class Application;
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
    bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
    UpdateItem *m_rootItem;

public Q_SLOTS:
    void packageChanged();

Q_SIGNALS:
    void checkApp(Application *app, bool checked);
    void checkApps(QList<Application *> apps, bool checked);
};

#endif // UPDATEMODEL_H
