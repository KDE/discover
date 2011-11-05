#include "StatusFilter.h"

// KDE includes
#include <KIcon>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Package>

// Own includes
#include "../libmuon/MuonStrings.h"

StatusFilter::StatusFilter(QObject *parent)
    : FilterModel(parent)
{
}

void StatusFilter::populate()
{
    QVector<QStandardItem *> items;
    QStandardItem *defaultItem = new QStandardItem;
    items.append(defaultItem);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));

    QStandardItem *installedItem = new QStandardItem;
    items.append(installedItem);
    installedItem->setIcon(KIcon("download"));
    installedItem->setText(MuonStrings::global()->packageStateName(QApt::Package::Installed));
    installedItem->setData(QApt::Package::Installed);

    QStandardItem *notInstalledItem = new QStandardItem;
    items.append(notInstalledItem);
    notInstalledItem->setIcon(KIcon("application-x-deb"));
    notInstalledItem->setText(MuonStrings::global()->packageStateName(QApt::Package::NotInstalled));
    notInstalledItem->setData(QApt::Package::NotInstalled);

    QStandardItem *upgradeableItem = new QStandardItem;
    items.append(upgradeableItem);
    upgradeableItem->setIcon(KIcon("system-software-update"));
    upgradeableItem->setText(MuonStrings::global()->packageStateName(QApt::Package::Upgradeable));
    upgradeableItem->setData(QApt::Package::Upgradeable);

    QStandardItem *brokenItem = new QStandardItem;
    items.append(brokenItem);
    brokenItem->setIcon(KIcon("dialog-cancel"));
    brokenItem->setText(MuonStrings::global()->packageStateName(QApt::Package::NowBroken));
    brokenItem->setData(QApt::Package::NowBroken);

    QStandardItem *purgeableItem = new QStandardItem;
    items.append(purgeableItem);
    purgeableItem->setIcon(KIcon("user-trash-full"));
    purgeableItem->setText(MuonStrings::global()->packageStateName(QApt::Package::ResidualConfig));
    purgeableItem->setData(QApt::Package::ResidualConfig);

    QStandardItem *autoRemoveItem = new QStandardItem;
    items.append(autoRemoveItem);
    autoRemoveItem->setIcon(KIcon("archive-remove"));
    autoRemoveItem->setText(MuonStrings::global()->packageStateName(QApt::Package::IsGarbage));
    autoRemoveItem->setData(QApt::Package::IsGarbage);

    QStandardItem *lockedItem = new QStandardItem;
    items.append(lockedItem);
    lockedItem->setIcon(KIcon("object-locked"));
    lockedItem->setText(MuonStrings::global()->packageStateName(QApt::Package::IsPinned));
    lockedItem->setData(QApt::Package::IsPinned);

    for (QStandardItem *item : items) {
        item->setEditable(false);
        appendRow(item);
    }
}
