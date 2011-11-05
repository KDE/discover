#include "OriginFilter.h"

// KDE includes
#include <KIcon>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Backend>

OriginFilter::OriginFilter(QObject *parent, QApt::Backend *backend)
    : FilterModel(parent)
    , m_backend(backend)
{
}

void OriginFilter::populate()
{
    QStringList originLabels = m_backend->originLabels();

    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));
    appendRow(defaultItem);

    foreach(const QString &originLabel, originLabels) {
        QStandardItem *originItem = new QStandardItem;
        originItem->setEditable(false);
        originItem->setText(originLabel);
        appendRow(originItem);
    }
}
