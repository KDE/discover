/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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
#include <QtGui/QHeaderView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>

// KDE includes
#include <KIcon>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KVBox>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../libmuon/DetailsWidget.h"
#include "../libmuon/PackageModel/PackageModel.h"
#include "../libmuon/PackageModel/PackageProxyModel.h"
#include "../libmuon/PackageModel/PackageView.h"
#include "../libmuon/PackageModel/PackageDelegate.h"

bool packageNameLessThan(QApt::Package *p1, QApt::Package *p2)
{
     return p1->latin1Name() < p2->latin1Name();
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

    m_searchEdit = new KLineEdit(topVBox);
    m_searchEdit->setClickMessage(i18nc("@label Line edit click message", "Search"));
    m_searchEdit->setClearButtonShown(true);
    m_searchEdit->setEnabled(false);
    m_searchEdit->hide(); // Off by default, use showSearchEdit() to show

    m_packageView = new PackageView(topVBox);
    m_packageView->setModel(m_proxyModel);
    m_packageView->setItemDelegate(delegate);
    m_packageView->header()->setResizeMode(0, QHeaderView::Stretch);

    KVBox *bottomVBox = new KVBox(this);

    m_detailsWidget = new DetailsWidget(bottomVBox);
    connect(m_detailsWidget, SIGNAL(setInstall(QApt::Package *)),
            this, SLOT(setInstall(QApt::Package *)));
    connect(m_detailsWidget, SIGNAL(setRemove(QApt::Package *)),
            this, SLOT(setRemove(QApt::Package *)));
    connect(m_detailsWidget, SIGNAL(setUpgrade(QApt::Package *)),
            this, SLOT(setUpgrade(QApt::Package *)));
    connect(m_detailsWidget, SIGNAL(setReInstall(QApt::Package *)),
            this, SLOT(setReInstall(QApt::Package *)));
    connect(m_detailsWidget, SIGNAL(setKeep(QApt::Package *)),
            this, SLOT(setKeep(QApt::Package *)));
    connect(m_detailsWidget, SIGNAL(setPurge(QApt::Package *)),
            this, SLOT(setPurge(QApt::Package *)));

    m_busyWidget = new KPixmapSequenceOverlayPainter(this);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_packageView->viewport());

    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_busyWidget->start();

    connect(m_packageView, SIGNAL(currentPackageChanged(const QModelIndex &)),
            this, SLOT(packageActivated(const QModelIndex &)));
    connect(m_packageView, SIGNAL(selectionEmpty()), m_detailsWidget, SLOT(hide()));
    connect(m_searchEdit, SIGNAL(textChanged(const QString &)), m_searchTimer, SLOT(start()));

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(topVBox);
    splitter->addWidget(bottomVBox);
}

PackageWidget::~PackageWidget()
{
}

void PackageWidget::setHeaderText(const QString &text)
{
    m_headerLabel->setText(text);
}

void PackageWidget::setPackagesType(int type)
{
    m_packagesType = type;

    switch (m_packagesType) {
        case AvailablePackages:
            break;
        case UpgradeablePackages: {
            QApt::Package::State state =
                        (QApt::Package::State)(QApt::Package::Upgradeable | QApt::Package::ToInstall |
                        QApt::Package::ToReInstall | QApt::Package::ToUpgrade |
                        QApt::Package::ToDowngrade | QApt::Package::ToRemove |
                        QApt::Package::ToPurge);
            m_proxyModel->setStateFilter(state);
            break;
        }
        case MarkedPackages: {
            QApt::Package::State state =
                          (QApt::Package::State)(QApt::Package::ToInstall |
                          QApt::Package::ToReInstall | QApt::Package::ToUpgrade |
                          QApt::Package::ToDowngrade | QApt::Package::ToRemove |
                          QApt::Package::ToPurge);
            m_proxyModel->setStateFilter(state);
            break;
        }
    }
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

    m_detailsWidget->setBackend(backend);
    m_proxyModel->setBackend(m_backend);
    m_packageView->setSortingEnabled(true);
    QApt::PackageList packageList = m_backend->availablePackages();
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

void PackageWidget::setSortedPackages()
{
    QApt::PackageList packageList = m_watcher->future().result();
    m_model->setPackages(packageList);
    m_searchEdit->setEnabled(true);
    m_busyWidget->stop();
    QApplication::restoreOverrideCursor();
}

void PackageWidget::startSearch()
{
    m_proxyModel->search(m_searchEdit->text());
}

bool PackageWidget::confirmEssentialRemoval()
{
    QString text = i18nc("@label", "Removing this package may break your system. Are you sure you want to remove it?");
    QString title = i18nc("@label", "Warning - Removing Important Package");
    int result = KMessageBox::Cancel;

    result = KMessageBox::warningContinueCancel(this, text, title, KStandardGuiItem::cont(),
             KStandardGuiItem::cancel(), QString(), KMessageBox::Dangerous);

    switch (result) {
    case KMessageBox::Continue:
        return true;
        break;
    case KMessageBox::Cancel:
    default:
        return false;
        break;
    }
}

void PackageWidget::setInstall(QApt::Package *package)
{
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    if (!package->availableVersion().isEmpty()) {
        package->setInstall();
    }

    if (package->wouldBreak()) {
        showBrokenReason(package);
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void PackageWidget::setRemove(QApt::Package *package)
{
    bool remove = true;
    if (package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        m_oldCacheState = m_backend->currentCacheState();
        m_backend->saveCacheState();
        package->setRemove();

        if (package->wouldBreak()) {
            showBrokenReason(package);
            m_backend->restoreCacheState(m_oldCacheState);
        }
    }
}

void PackageWidget::setUpgrade(QApt::Package *package)
{
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    package->setInstall();

    if (package->wouldBreak()) {
        showBrokenReason(package);
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void PackageWidget::setReInstall(QApt::Package *package)
{
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    package->setReInstall();

    if (package->wouldBreak()) {
        showBrokenReason(package);
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void PackageWidget::setPurge(QApt::Package *package)
{
    bool remove = true;
    if (package->state() & QApt::Package::IsImportant) {
        remove = confirmEssentialRemoval();
    }

    if (remove) {
        m_backend->saveCacheState();
        m_oldCacheState = m_backend->currentCacheState();
        package->setPurge();

        if (package->wouldBreak()) {
            showBrokenReason(package);
            m_backend->restoreCacheState(m_oldCacheState);
        }
    }
}

void PackageWidget::setKeep(QApt::Package *package)
{
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    package->setKeep();

    if (package->wouldBreak()) {
        m_backend->restoreCacheState(m_oldCacheState);
    }
}

void PackageWidget::showBrokenReason(QApt::Package *package)
{
    QHash<int, QHash<QString, QVariantMap> > failedReasons = package->brokenReason();
    QString reason;
    QString dialogText = i18nc("@label", "The \"%1\" package could not be marked for installation or upgrade:",
                               package->latin1Name());
    dialogText += '\n';
    QString title = i18nc("@title:window", "Unable to Mark Package");

    QHash<int, QHash<QString, QVariantMap> >::const_iterator reasonIter = failedReasons.constBegin();
    QHash<int, QHash<QString, QVariantMap> >::const_iterator end = failedReasons.constEnd();
    while (reasonIter != end) {
        QApt::BrokenReason failType = (QApt::BrokenReason)reasonIter.key();
        QHash<QString, QVariantMap> failReason = reasonIter.value();
        dialogText += digestReason(package, failType, failReason);

        reasonIter++;
    }

    KMessageBox::information(this, dialogText, title);
}

QString PackageWidget::digestReason(QApt::Package *pkg, QApt::BrokenReason failType, QHash<QString, QVariantMap> failReason)
{
    QHash<QString, QVariantMap>::const_iterator packageIter = failReason.constBegin();
    QHash<QString, QVariantMap>::const_iterator end = failReason.constEnd();
    QString reason;

    switch (failType) {
    case QApt::ParentNotInstallable: {
        reason += '\t';
        reason = i18nc("@label", "The \"%1\" package has no available version, but exists in the database.\n"
                       "\tThis typically means that the package was mentioned in a dependency and "
                       "never uploaded, has been obsoleted, or is not available from the currently-enabled "
                       "repositories.", pkg->latin1Name());
        break;
    }
    case QApt::WrongCandidateVersion: {
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            QString requiredVersion = packageIter.value()["RequiredVersion"].toString();
            QString candidateVersion = packageIter.value()["CandidateVersion"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt 0.1, but 0.2 is to be installed",
                                "%1: %2 %3, but %4 is to be installed",
                                relation, package, requiredVersion, candidateVersion);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt 0.1, but 0.2 is to be installed",
                                        "or %1 %2, but %3 is to be installed",
                                        package, requiredVersion, candidateVersion));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    }
    case QApt::DepNotInstallable: {
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt, but is not installable",
                                "%1: %2, but it is not installable",
                                relation, package);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt, but is not installable",
                                        "or %1, but is not installable",
                                        package));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    }
    case QApt::VirtualPackage:
        while (packageIter != end) {
            QString package = packageIter.key();
            QString relation = packageIter.value()["Relation"].toString();
            bool isFirstOr = !packageIter.value()["IsFirstOr"].toBool();

            if (isFirstOr) {
                reason += '\t';
                reason += i18nc("@label Example: Depends: libqapt, but it is a virtual package",
                                "%1: %2, but it is a virtual package",
                                relation, package);
                reason += '\n';
            } else {
                reason += '\t';
                reason += QString(i18nc("@label Example: or libqapt, but it is a virtual package",
                                        "or %1, but it is a virtual package",
                                        package));
                reason += '\n';
            }
            packageIter++;
        }

        return reason;
        break;
    default:
        break;
    }

    return reason;
}

#include "PackageWidget.moc"
