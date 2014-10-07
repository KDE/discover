/***************************************************************************
 *   Copyright © 2010,2011 Jonathan Thomas <echidnaman@kubuntu.org>        *
 *   Copyright © 2010 Guillaume Martres <smarter@ubuntu.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "PackageWidget.h"

// Qt includes
#include <QtConcurrentRun>
#include <QApplication>
#include <QtCore/QTimer>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QSplitter>

// KDE includes
#include <KComboBox>
#include <KHBox>
#include <KLineEdit>
#include <KLocalizedString>
#include <KMenu>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KVBox>
#include <KIconLoader>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/DependencyInfo>
#include <LibQApt/MarkingErrorInfo>

// Own includes
#include "../libmuonapt/ChangesDialog.h"
#include "DetailsWidget.h"
#include "MuonSettings.h"
#include "PackageModel.h"
#include "PackageProxyModel.h"
#include "PackageView.h"
#include "PackageDelegate.h"

#define NUM_COLUMNS 3 // If this is changed change PackageView.cpp value as well

bool packageNameLessThan(QApt::Package *p1, QApt::Package *p2)
{
     return p1->name() < p2->name();
}

QApt::PackageList sortPackages(QApt::PackageList list)
{
    qSort(list.begin(), list.end(), packageNameLessThan);
    return list;
}

PackageWidget::PackageWidget(QWidget *parent)
        : KVBox(parent)
        , m_backend(0)
        , m_headerLabel(0)
        , m_searchEdit(0)
        , m_packagesType(0)
        , m_stop(false)
{
    m_watcher = new QFutureWatcher<QList<QApt::Package*> >(this);
    connect(m_watcher, SIGNAL(finished()), this, SLOT(setSortedPackages()));

    m_model = new PackageModel(this);
    PackageDelegate *delegate = new PackageDelegate(this);
    m_proxyModel = new PackageProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    KVBox *topVBox = new KVBox;

    m_headerLabel = new QLabel(topVBox);
    m_headerLabel->setTextFormat(Qt::RichText);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(300);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(startSearch()));

    setupActions();

    m_searchEdit = new KLineEdit(topVBox);
    m_searchEdit->setEnabled(false);
    m_searchEdit->setClickMessage(i18nc("@label Line edit click message", "Search"));
    m_searchEdit->setClearButtonShown(true);
    m_searchEdit->hide(); // Off by default, use showSearchEdit() to show

    m_packageView = new PackageView(topVBox);
    m_packageView->setModel(m_proxyModel);
    m_packageView->setItemDelegate(delegate);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);

    KVBox *bottomVBox = new KVBox(this);

    m_detailsWidget = new DetailsWidget(bottomVBox);
    connect(m_detailsWidget, SIGNAL(setInstall(QApt::Package*)),
            this, SLOT(setInstall(QApt::Package*)));
    connect(m_detailsWidget, SIGNAL(setRemove(QApt::Package*)),
            this, SLOT(setRemove(QApt::Package*)));
    connect(m_detailsWidget, SIGNAL(setUpgrade(QApt::Package*)),
            this, SLOT(setUpgrade(QApt::Package*)));
    connect(m_detailsWidget, SIGNAL(setReInstall(QApt::Package*)),
            this, SLOT(setReInstall(QApt::Package*)));
    connect(m_detailsWidget, SIGNAL(setKeep(QApt::Package*)),
            this, SLOT(setKeep(QApt::Package*)));
    connect(m_detailsWidget, SIGNAL(setPurge(QApt::Package*)),
            this, SLOT(setPurge(QApt::Package*)));

    m_busyWidget = new KPixmapSequenceOverlayPainter(this);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_packageView->viewport());

    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_busyWidget->start();

    connect(m_packageView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequested(QPoint)));
    connect(m_packageView, SIGNAL(currentPackageChanged(QModelIndex)),
            this, SLOT(packageActivated(QModelIndex)));
    connect(m_packageView, SIGNAL(selectionEmpty()), m_detailsWidget, SLOT(hide()));
    connect(m_packageView, SIGNAL(selectionMulti()), m_detailsWidget, SLOT(emitHideButtons()));
    connect(m_packageView->header(), SIGNAL(sectionClicked(int)),
            this, SLOT(sectionClicked(int)));
    connect(m_searchEdit, SIGNAL(textChanged(QString)), m_searchTimer, SLOT(start()));

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(topVBox);
    splitter->addWidget(bottomVBox);
}

void PackageWidget::setupActions()
{
    m_installAction = new QAction(this);
    m_installAction->setIcon(QIcon::fromTheme("download"));
    m_installAction->setText(i18nc("@action:inmenu", "Mark for Installation"));
    connect(m_installAction, SIGNAL(triggered()), this, SLOT(setPackagesInstall()));

    m_removeAction = new QAction(this);
    m_removeAction->setIcon(QIcon::fromTheme("edit-delete"));
    m_removeAction->setText(i18nc("@action:button", "Mark for Removal"));
    connect(m_removeAction, SIGNAL(triggered()), this, SLOT(setPackagesRemove()));

    m_upgradeAction = new QAction(this);
    m_upgradeAction->setIcon(QIcon::fromTheme("system-software-update"));
    m_upgradeAction->setText(i18nc("@action:button", "Mark for Upgrade"));
    connect(m_upgradeAction, SIGNAL(triggered()), this, SLOT(setPackagesUpgrade()));

    m_reinstallAction = new QAction(this);
    m_reinstallAction->setIcon(QIcon::fromTheme("view-refresh"));
    m_reinstallAction->setText(i18nc("@action:button", "Mark for Reinstallation"));
    connect(m_reinstallAction, SIGNAL(triggered()), this, SLOT(setPackagesReInstall()));

    m_purgeAction = new QAction(this);
    m_purgeAction->setIcon(QIcon::fromTheme("edit-delete-shred"));
    m_purgeAction->setText(i18nc("@action:button", "Mark for Purge"));
    connect(m_purgeAction, SIGNAL(triggered()), this, SLOT(setPackagesPurge()));

    m_keepAction = new QAction(this);
    m_keepAction->setIcon(QIcon::fromTheme("dialog-cancel"));
    m_keepAction->setText(i18nc("@action:button", "Unmark"));
    connect(m_keepAction, SIGNAL(triggered()), this, SLOT(setPackagesKeep()));

    m_lockAction = new QAction(this);
    m_lockAction->setCheckable(true);
    m_lockAction->setIcon(QIcon::fromTheme("object-locked"));
    m_lockAction->setText(i18nc("@action:button", "Lock Package at Current Version"));
    connect(m_lockAction, SIGNAL(triggered(bool)), this, SLOT(setPackagesLocked(bool)));
}

void PackageWidget::setHeaderText(const QString &text)
{
    m_headerLabel->setText(text);
}

void PackageWidget::setPackagesType(int type)
{
    m_packagesType = type;

    QApt::Package::State state = (QApt::Package::State)0;


    if (type == MarkedPackages) {
        state = (QApt::Package::State)(QApt::Package::ToInstall |
                 QApt::Package::ToReInstall | QApt::Package::ToUpgrade |
                 QApt::Package::ToDowngrade | QApt::Package::ToRemove |
                 QApt::Package::ToPurge);
    }

    m_proxyModel->setStateFilter(state);
}

void PackageWidget::hideHeaderLabel()
{
    m_headerLabel->hide();
}

void PackageWidget::showSearchEdit()
{
    m_searchEdit->show();
}

void PackageWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()), m_detailsWidget, SLOT(refreshTabs()));
    connect(m_backend, SIGNAL(packageChanged()), m_model, SLOT(externalDataChanged()));

    m_detailsWidget->setBackend(backend);
    m_proxyModel->setBackend(m_backend);
    m_packageView->setSortingEnabled(true);
    QApt::PackageList packageList = m_backend->availablePackages();
    emit doneSortingPackages(false);
    QFuture<QList<QApt::Package*> > future = QtConcurrent::run(sortPackages, packageList);
    m_watcher->setFuture(future);
    m_packageView->updateView();
}

void PackageWidget::reload()
{
    m_detailsWidget->clear();
    m_model->clear();
    m_proxyModel->clear();
    m_proxyModel->setSourceModel(0);
    m_busyWidget->start();
    m_backend->reloadCache();
    QApt::PackageList packageList = m_backend->availablePackages();
    QFuture<QList<QApt::Package*> > future = QtConcurrent::run(sortPackages, packageList);
    m_watcher->setFuture(future);
    m_proxyModel->setSourceModel(m_model);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);
}

void PackageWidget::packageActivated(const QModelIndex &index)
{
    QApt::Package *package = m_proxyModel->packageAt(index);
    if (package == 0) {
        m_detailsWidget->hide();
        return;
    }
    m_detailsWidget->setPackage(package);
}

void PackageWidget::contextMenuRequested(const QPoint &pos)
{
    KMenu menu;

    menu.addAction(m_installAction);
    menu.addAction(m_removeAction);
    menu.addAction(m_upgradeAction);
    menu.addAction(m_reinstallAction);
    menu.addAction(m_purgeAction);
    menu.addAction(m_keepAction);
    menu.addSeparator();
    menu.addAction(m_lockAction);

    const QModelIndexList selected = m_packageView->selectionModel()->selectedIndexes();

    if (!selected.size()) {
        return;
    }

    // Divide by the number of columns
    if (selected.size()/NUM_COLUMNS == 1) {
        int state = m_proxyModel->packageAt(selected.first())->state();
        bool upgradeable = (state & QApt::Package::Upgradeable);

        if (state & QApt::Package::Installed) {
            m_installAction->setEnabled(false);

            if (upgradeable) {
                m_upgradeAction->setEnabled(true);
            } else {
                m_upgradeAction->setEnabled(false);
            }
            if (state & (QApt::Package::NotDownloadable) || upgradeable) {
                m_reinstallAction->setEnabled(false);
            } else {
                m_reinstallAction->setEnabled(true);
            }
            if (state & (QApt::Package::ToReInstall |
			 QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
			 QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
		m_removeAction->setEnabled(false);
		m_purgeAction->setEnabled(false);
		m_keepAction->setEnabled(true);
	    }
            else {
		m_removeAction->setEnabled(true);
		m_keepAction->setEnabled(false);
		m_purgeAction->setEnabled(true);
	    }

        } else if (state & QApt::Package::ResidualConfig) {
            m_purgeAction->setEnabled(true);
            m_installAction->setEnabled(true);
            m_removeAction->setEnabled(false);
            m_upgradeAction->setEnabled(false);
            m_reinstallAction->setEnabled(false);
            m_keepAction->setEnabled(false);
	} else if (state & (QApt::Package::ToInstall | QApt::Package::ToReInstall |
                            QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
                            QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
          m_purgeAction->setEnabled(false);
	  m_installAction->setEnabled(false);
	  m_removeAction->setEnabled(false);
	  m_upgradeAction->setEnabled(false);
	  m_reinstallAction->setEnabled(false);
	  m_keepAction->setEnabled(true);
	} else {
            m_purgeAction->setEnabled(false);
            m_installAction->setEnabled(true);
            m_removeAction->setEnabled(false);
            m_upgradeAction->setEnabled(false);
            m_reinstallAction->setEnabled(false);
            m_keepAction->setEnabled(false);
        }

        if (state & QApt::Package::IsPinned) {
            m_lockAction->setChecked(true);
            m_lockAction->setText(i18nc("@action:button", "Unlock package"));
            m_lockAction->setIcon(QIcon::fromTheme("object-unlocked"));
        } else {
            m_lockAction->setChecked(false);
            m_lockAction->setText(i18nc("@action:button", "Lock at Current Version"));
            m_lockAction->setIcon(QIcon::fromTheme("object-locked"));
        }
    } else {
        m_installAction->setEnabled(true);
        m_removeAction->setEnabled(true);
        m_upgradeAction->setEnabled(true);
        m_reinstallAction->setEnabled(true);
        m_purgeAction->setEnabled(true);
        m_keepAction->setEnabled(true);
    }

    menu.exec(m_packageView->mapToGlobal(pos));
}

void PackageWidget::setSortedPackages()
{
    QApt::PackageList packageList = m_watcher->future().result();
    m_model->setPackages(packageList);
    m_searchEdit->setEnabled(true);
    m_busyWidget->stop();
    QApplication::restoreOverrideCursor();
    emit doneSortingPackages(true);
}

void PackageWidget::sectionClicked(int section)
{
    if (section == 0) {
        m_proxyModel->setSortByRelevancy(false);
    }
}

void PackageWidget::startSearch()
{
    if (m_proxyModel->sourceModel()) {
        m_proxyModel->search(m_searchEdit->text());
    }
}

void PackageWidget::invalidateFilter()
{
    if (m_proxyModel) {
        m_proxyModel->invalidate();
    }
}

bool PackageWidget::confirmEssentialRemoval()
{
    QString text = i18nc("@label", "Removing this package may break your system. Are you sure you want to remove it?");
    QString title = i18nc("@label", "Warning - Removing Important Package");
    int result = KMessageBox::Cancel;

    result = KMessageBox::warningContinueCancel(this, text, title, KStandardGuiItem::cont(),
             KStandardGuiItem::cancel(), QString(), KMessageBox::Dangerous);

    return (result == KMessageBox::Continue) ? true : false;
}

void PackageWidget::saveState()
{
    if (!m_backend->areEventsCompressed()) {
        m_oldCacheState = m_backend->currentCacheState();
        m_backend->saveCacheState();
    }
}

void PackageWidget::handleBreakage(QApt::Package *package)
{
    if (package->wouldBreak() || m_backend->isBroken()) {
        showBrokenReason(package);
        m_backend->restoreCacheState(m_oldCacheState);
        m_stop = true;
    }
}

void PackageWidget::actOnPackages(QApt::Package::State action)
{
    const QApt::PackageList packages = selectedPackages();

    if (packages.isEmpty())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    saveState();
    m_backend->setCompressEvents(true);
    m_stop = false;

    for (QApt::Package *package : packages) {
        if (m_stop) {
            m_backend->setCompressEvents(false);
            QApplication::restoreOverrideCursor();
            break;
        }

        switch (action) {
        case QApt::Package::ToInstall:
            setInstall(package);
            break;
        case QApt::Package::ToRemove:
            setRemove(package);
            break;
        case QApt::Package::ToUpgrade:
            setUpgrade(package);
            break;
        case QApt::Package::ToReInstall:
            if (package->isInstalled())
                setReInstall(package);
            break;
        case QApt::Package::ToKeep:
            setKeep(package);
            break;
        case QApt::Package::ToPurge:
            setPurge(package);
            break;
        default:
            break;
        }
    }

    emit packageChanged();

    m_backend->setCompressEvents(false);
    QApplication::restoreOverrideCursor();

    checkChanges();
}

void PackageWidget::setInstall(QApt::Package *package)
{
    saveState();

    if (!package->availableVersion().isEmpty()) {
        package->setInstall();
    }

    // Check for/handle breakage
    handleBreakage(package);
    checkChanges();
}

void PackageWidget::setPackagesInstall()
{
    actOnPackages(QApt::Package::ToInstall);
}

void PackageWidget::setRemove(QApt::Package *package)
{
    bool remove = true;
    if (package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        saveState();
        package->setRemove();

        handleBreakage(package);
        checkChanges();
    }
}

void PackageWidget::setPackagesRemove()
{
    actOnPackages(QApt::Package::ToRemove);
}

void PackageWidget::setUpgrade(QApt::Package *package)
{
    setInstall(package);
}

void PackageWidget::setPackagesUpgrade()
{
    actOnPackages(QApt::Package::ToUpgrade);
}

void PackageWidget::setReInstall(QApt::Package *package)
{
    saveState();

    package->setReInstall();

    handleBreakage(package);
    checkChanges();
}

void PackageWidget::setPackagesReInstall()
{
    actOnPackages(QApt::Package::ToReInstall);
}

void PackageWidget::setPurge(QApt::Package *package)
{
    bool remove = true;
    if (package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        saveState();
        package->setPurge();

        handleBreakage(package);
        checkChanges();
    }
}

void PackageWidget::setPackagesPurge()
{
    actOnPackages(QApt::Package::ToPurge);
}

void PackageWidget::setKeep(QApt::Package *package)
{
    saveState();
    package->setKeep();

    handleBreakage(package);
    checkChanges();
}

void PackageWidget::setPackagesKeep()
{
    actOnPackages(QApt::Package::ToKeep);
}

void PackageWidget::setPackagesLocked(bool lock)
{
    const QApt::PackageList packages = selectedPackages();

    if (packages.isEmpty())
        return;

    for (QApt::Package *package : packages) {
        if (!m_backend->setPackagePinned(package, lock)) {
            QString title = i18nc("@title:window", "Failed to Lock Package");
            QString text = i18nc("@info Error text", "The package %1 could not "
                                 "be locked. Failed to write lock file.",
                                 package->name());
            KMessageBox::error(this, text, title);
        }
    }

    reload();
}

void PackageWidget::checkChanges()
{
    MuonSettings *settings = MuonSettings::self();

    if (m_backend->areEventsCompressed() || !settings->askChanges())
        return;

    auto changes = m_backend->stateChanges(m_oldCacheState, selectedPackages());

    if (changes.isEmpty())
        return;

    ChangesDialog *dialog = new ChangesDialog(this, changes);
    int res = dialog->exec();

    if (res != QDialog::Accepted)
        m_backend->restoreCacheState(m_oldCacheState);
}

QApt::PackageList PackageWidget::selectedPackages()
{
    const QModelIndexList selected = m_packageView->selectionModel()->selectedIndexes();

    if (selected.isEmpty()) {
        return QApt::PackageList();
    }

    // There are three indexes per row, so we want a duplicate-less set of packages
    QSet<QApt::Package *> packages;
    for (const QModelIndex &index : selected) {
        packages << m_proxyModel->packageAt(index);
    }

    return packages.toList();
}

void PackageWidget::showBrokenReason(QApt::Package *package)
{
    QList<QApt::MarkingErrorInfo> failedReasons = package->brokenReason();
    QString dialogText = i18nc("@label", "The \"%1\" package could not be marked for installation or upgrade:",
                               package->name());
    dialogText += '\n';
    QString title = i18nc("@title:window", "Unable to Mark Package");

    for (const QApt::MarkingErrorInfo &reason : failedReasons)
        dialogText += digestReason(package, reason);

    KMessageBox::information(this, dialogText, title);
}

QString PackageWidget::digestReason(QApt::Package *pkg, const QApt::MarkingErrorInfo &info)
{
    QString reason;
    QString relation = QApt::DependencyInfo::typeName(info.errorInfo().dependencyType());

    reason += '\t';

    switch (info.errorType()) {
    case QApt::ParentNotInstallable:
        reason += i18nc("@label", "The \"%1\" package has no available version, but exists in the database.\n"
                        "\tThis typically means that the package was mentioned in a dependency and "
                        "never uploaded, has been obsoleted, or is not available from the currently-enabled "
                        "repositories.", pkg->name());
        break;
    case QApt::WrongCandidateVersion:
        reason += i18nc("@label Example: Depends: libqapt 0.1, but 0.2 is to be installed",
                        "%1: %2 %3, but %4 is to be installed",
                        relation, pkg->name(), info.errorInfo().packageVersion(),
                        pkg->availableVersion());
        break;
    case QApt::DepNotInstallable:
        reason += i18nc("@label Example: Depends: libqapt, but is not installable",
                        "%1: %2, but it is not installable",
                        relation, pkg->name());
        break;
    case QApt::VirtualPackage:
        reason += i18nc("@label Example: Depends: libqapt, but it is a virtual package",
                        "%1: %2, but it is a virtual package",
                        relation, pkg->name());
        break;
    default:
        break;
    }

    reason += '\n';

    return reason;
}
