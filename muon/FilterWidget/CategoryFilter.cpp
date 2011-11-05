#include "CategoryFilter.h"

// Qt includes
#include <QtCore/QSet>

// KDE includes
#include <KIcon>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/MuonStrings.h"

CategoryFilter::CategoryFilter(QObject *parent, QApt::Backend *backend)
    : FilterModel(parent)
    , m_backend(backend)
{
}

void CategoryFilter::populate()
{
    QApt::GroupList groups = m_backend->availableGroups();
    QSet<QString> groupSet;

    foreach(const QApt::Group &group, groups) {
        QString groupName = MuonStrings::global()->groupName(group);

        if (!groupName.isEmpty()) {
            groupSet << groupName;
        }
    }

    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));
    appendRow(defaultItem);

    QStringList groupList = groupSet.toList();
    qSort(groupList);

    foreach(const QString &group, groupList) {
        QStandardItem *groupItem = new QStandardItem;
        groupItem->setEditable(false);
        groupItem->setText(group);
        appendRow(groupItem);
    }
}
