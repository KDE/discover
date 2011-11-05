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
    QStandardItem *defaultItem = new QStandardItem;
    defaultItem->setEditable(false);
    defaultItem->setIcon(KIcon("bookmark-new-list"));
    defaultItem->setText(i18nc("@item:inlistbox Item that resets the filter to \"all\"", "All"));

    QStandardItem *installedItem = new QStandardItem;
    installedItem->setEditable(false);
    installedItem->setIcon(KIcon("download"));
    installedItem->setText(MuonStrings::global()->packageStateName(QApt::Package::Installed));
    installedItem->setData(QApt::Package::Installed);

    QStandardItem *notInstalledItem = new QStandardItem;
    notInstalledItem->setEditable(false);
    notInstalledItem->setIcon(KIcon("application-x-deb"));
    notInstalledItem->setText(MuonStrings::global()->packageStateName(QApt::Package::NotInstalled));
    notInstalledItem->setData(QApt::Package::NotInstalled);

    QStandardItem *upgradeableItem = new QStandardItem;
    upgradeableItem->setEditable(false);
    upgradeableItem->setIcon(KIcon("system-software-update"));
    upgradeableItem->setText(MuonStrings::global()->packageStateName(QApt::Package::Upgradeable));
    upgradeableItem->setData(QApt::Package::Upgradeable);

    QStandardItem *brokenItem = new QStandardItem;
    brokenItem->setEditable(false);
    brokenItem->setIcon(KIcon("dialog-cancel"));
    brokenItem->setText(MuonStrings::global()->packageStateName(QApt::Package::NowBroken));
    brokenItem->setData(QApt::Package::NowBroken);

    QStandardItem *purgeableItem = new QStandardItem;
    purgeableItem->setEditable(false);
    purgeableItem->setIcon(KIcon("user-trash-full"));
    purgeableItem->setText(MuonStrings::global()->packageStateName(QApt::Package::ResidualConfig));
    purgeableItem->setData(QApt::Package::ResidualConfig);

    QStandardItem *autoRemoveItem = new QStandardItem;
    autoRemoveItem->setEditable(false);
    autoRemoveItem->setIcon(KIcon("archive-remove"));
    autoRemoveItem->setText(MuonStrings::global()->packageStateName(QApt::Package::IsGarbage));
    autoRemoveItem->setData(QApt::Package::IsGarbage);

    QStandardItem *lockedItem = new QStandardItem;
    lockedItem->setEditable(false);
    lockedItem->setIcon(KIcon("object-locked"));
    lockedItem->setText(MuonStrings::global()->packageStateName(QApt::Package::IsPinned));
    lockedItem->setData(QApt::Package::IsPinned);

    appendRow(defaultItem);
    appendRow(installedItem);
    appendRow(notInstalledItem);
    appendRow(upgradeableItem);
    appendRow(brokenItem);
    appendRow(purgeableItem);
    appendRow(autoRemoveItem);
    appendRow(lockedItem);
}
