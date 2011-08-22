#ifndef UPDATEITEM_H
#define UPDATEITEM_H

// Qt includes
#include <QtCore/QList>
#include <QtCore/QString>

#include <KIcon>

class Application;

class UpdateItem
{
public:
    enum ItemType {
        InvalidItem = 0,
        RootItem,
        CategoryItem,
        ApplicationItem
    };

    UpdateItem();
    UpdateItem(const QString &categoryName,
               const KIcon &categoryIcon);
    UpdateItem(Application *app, UpdateItem *parent = 0);

    ~UpdateItem();

    UpdateItem *parent() const;
    void setParent(UpdateItem *parent);

    void appendChild(UpdateItem *child);
    QList<UpdateItem *> children() const;
    UpdateItem *child(int row) const;
    int childCount() const;
    int row() const;

    Application *app() const;
    QString name() const;
    KIcon icon() const;
    qint64 size() const;
    Qt::CheckState checked() const;
    ItemType type() const;

private:
    Application *m_app;

    UpdateItem *m_parent;
    QList<UpdateItem *> m_children;
    QString m_categoryName;
    KIcon m_categoryIcon;
};

#endif // UPDATEITEM_H
