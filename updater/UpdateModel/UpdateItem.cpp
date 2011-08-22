#include "UpdateItem.h"

#include "../../installer/Application.h"

UpdateItem::UpdateItem()
    : m_app(0)
    , m_parent(0)
{
}

UpdateItem::UpdateItem(const QString &categoryName,
                       const KIcon &categoryIcon)
    : m_app(0)
    , m_parent(0)
    , m_categoryName(categoryName)
    , m_categoryIcon(categoryIcon)
{
}

UpdateItem::UpdateItem(Application *app, UpdateItem *parent)
    : m_app(app)
    , m_parent(parent)
{
}

UpdateItem::~UpdateItem()
{
    qDeleteAll(m_children);
}

UpdateItem *UpdateItem::parent() const
{
    return m_parent;
}

void UpdateItem::setParent(UpdateItem *parent)
{
    m_parent = parent;
}

void UpdateItem::appendChild(UpdateItem *child)
{
    child->setParent(this);
    m_children.append(child);
}

UpdateItem *UpdateItem::child(int row) const
{
    return m_children.value(row);
}

int UpdateItem::childCount() const
{
    return m_children.count();
}

int UpdateItem::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<UpdateItem*>(this));

    return 0;
}

Application *UpdateItem::app() const
{
    return m_app;
}

QString UpdateItem::name() const
{
    switch (type()) {
    case CategoryItem:
        return m_categoryName;
    case ApplicationItem:
        return m_app->name();
    default:
        return QString();
    }

    return QString();
}

KIcon UpdateItem::icon() const
{
    switch (type()) {
    case CategoryItem:
        return m_categoryIcon;
    case ApplicationItem:
        return KIcon(m_app->icon());
    default:
        return KIcon();
    }

    return KIcon();
}

qint64 UpdateItem::size() const
{
    int itemType = type();
    int size = 0;

    if (itemType == ApplicationItem) {
        size = m_app->package()->downloadSize();
    } else if (itemType == CategoryItem) {
        foreach (UpdateItem *item, m_children) {
            size += item->size();
        }
    }

    return size;
}

UpdateItem::ItemType UpdateItem::type() const
{
    if (!m_parent) {
        return RootItem;
    }

    // We now know we have a parent
    if (parent()->parent() == 0) {
        return CategoryItem;
    } else {
        return ApplicationItem;
    }
}
