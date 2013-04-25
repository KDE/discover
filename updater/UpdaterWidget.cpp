/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "UpdaterWidget.h"

// Qt includes
#include <QStandardItemModel>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <qaction.h>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KDebug>
#include <KMessageWidget>

// LibQApt includes
#include <LibQApt/Backend>

// Libmuon includes
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractResource.h>

//libmuonapt includes
#include "../libmuonapt/ChangesDialog.h"

// Own includes
#include "UpdateModel/UpdateModel.h"
#include "UpdateModel/UpdateItem.h"
#include "UpdateModel/UpdateDelegate.h"

UpdaterWidget::UpdaterWidget(QWidget *parent) :
    QStackedWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // First page (update view)
    QWidget *page1 = new QWidget(this);
    QVBoxLayout * page1Layout = new QVBoxLayout(page1);
    page1Layout->setMargin(0);
    page1Layout->setSpacing(0);
    page1->setLayout(page1Layout);

    QString text = i18nc("@label", "<em>Some packages were not marked for update.</em><p/>"
                            "The update of these packages need some others to be installed or removed.<p/>"
                            "Do you want to update those too?");
    QAction* action = new QAction(KIcon("dialog-ok-apply"), i18n("Mark All"), this);
    connect(action, SIGNAL(triggered(bool)), SLOT(markAllPackagesForUpgrade()));
    m_upgradesWidget = new KMessageWidget(this);
    m_upgradesWidget->setText(text);
    m_upgradesWidget->addAction(action);
    m_upgradesWidget->setCloseButtonVisible(true);
    m_upgradesWidget->setVisible(false);
    page1Layout->addWidget(m_upgradesWidget);

    m_updateModel = new UpdateModel(page1);

    connect(m_updateModel, SIGNAL(checkApps(QList<AbstractResource*>,bool)),
            this, SLOT(checkApps(QList<AbstractResource*>,bool)));

    m_updateView = new QTreeView(page1);
    m_updateView->setAlternatingRowColors(true);
    m_updateView->setModel(m_updateModel);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_updateView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_updateView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
    m_updateView->header()->setStretchLastSection(false);
    connect(m_updateView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    page1Layout->addWidget(m_updateView);

    m_busyWidget = new KPixmapSequenceOverlayPainter(page1);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_updateView->viewport());

    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_busyWidget->start();

    UpdateDelegate *delegate = new UpdateDelegate(m_updateView);
    m_updateView->setItemDelegate(delegate);

    // Second page (If no updates, show when last check or warn about lack of updates)
    QFrame *page2 = new QFrame(this);
    page2->setFrameShape(QFrame::StyledPanel);
    page2->setFrameShadow(QFrame::Sunken);
    page2->setBackgroundRole(QPalette::Base);
    QGridLayout *page2Layout = new QGridLayout(page2);
    page2->setLayout(page2Layout);

    m_updateStatusIcon = new QLabel(page2);
    m_notifyTitle = new QLabel(page2);
    m_notifyTitle->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_notifyDesc = new QLabel(page2);
    m_notifyDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    page2Layout->setColumnStretch(0, 1);
    page2Layout->addWidget(m_updateStatusIcon, 0, 1, 0, 1);
    page2Layout->addWidget(m_notifyTitle, 0, 2);
    page2Layout->addWidget(m_notifyDesc, 1, 2);
    page2Layout->setColumnStretch(3, 1);


    addWidget(page1);
    addWidget(page2);
    setCurrentWidget(page1);
}

void UpdaterWidget::setBackend(AbstractResourcesBackend *backend)
{
    m_appsBackend = backend;
    connect(m_appsBackend, SIGNAL(reloadFinished()),
            this, SLOT(populateUpdateModel()));
    m_backend = qobject_cast<QApt::Backend*>(backend->property("backend").value<QObject*>());
    connect(m_backend, SIGNAL(packageChanged()),
            m_updateModel, SLOT(packageChanged()));

    populateUpdateModel();
}

void UpdaterWidget::reload()
{
    m_updateModel->clear();
    QMetaObject::invokeMethod(m_appsBackend, "reload", Qt::QueuedConnection);

    setCurrentIndex(0);
}

void UpdaterWidget::populateUpdateModel()
{
    if (m_appsBackend->updatesCount()==0) {
        QApplication::restoreOverrideCursor();
        m_busyWidget->stop();
        checkUpToDate();
        return;
    }

    UpdateItem *securityItem = new UpdateItem(i18nc("@item:inlistbox", "Important Security Updates"),
                                              KIcon("security-medium"));

    UpdateItem *appItem = new UpdateItem(i18nc("@item:inlistbox", "Application Updates"),
                                          KIcon("applications-other"));

    UpdateItem *systemItem = new UpdateItem(i18nc("@item:inlistbox", "System Updates"),
                                             KIcon("applications-system"));
    
    QVector<AbstractResource*> resources = m_appsBackend->allResources();
    foreach(AbstractResource* res, resources) {
        if(res->state()==AbstractResource::Upgradeable) {
            UpdateItem *updateItem = new UpdateItem(res);
            QApt::Package* package = retrievePackage(res);

            if (!package) {
                kDebug() << "No package:" << res->name();
                continue;
            }
            
            bool securityFound = false;
            for (const QString &archive : package->archives()) {
                if (archive.contains(QLatin1String("security"))) {
                    securityFound = true;
                    break;
                }
            }
            
            if(!res->isTechnical()) {
                if (securityFound) {
                    securityItem->appendChild(updateItem);
                } else {
                    appItem->appendChild(updateItem);
                }
            } else {
                if (securityFound) {
                    securityItem->appendChild(updateItem);
                } else {
                    systemItem->appendChild(updateItem);
                }
            }
        }
    }

    // Add populated items to the model
    if (securityItem->childCount()) {
        securityItem->sort();
        m_updateModel->addItem(securityItem);
    } else {
        delete securityItem;
    }

    if (appItem->childCount()) {
        appItem->sort();
        m_updateModel->addItem(appItem);
    } else {
        delete appItem;
    }

    if (systemItem->childCount()) {
        systemItem->sort();
        m_updateModel->addItem(systemItem);
    } else {
        delete systemItem;
    }

    m_updateView->expand(m_updateModel->index(0,0)); // Expand apps category
    m_updateView->resizeColumnToContents(0);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_busyWidget->stop();
    QApplication::restoreOverrideCursor();
    m_backend->markPackagesForUpgrade();
    m_updateModel->updateCheckStates();

    checkAllMarked();
    checkUpToDate();
    emit modelPopulated();
}

void UpdaterWidget::checkApps(QList<AbstractResource*> apps, bool checked)
{
    QApt::PackageList list;
    foreach (AbstractResource *app, apps) {
        list << retrievePackage(app);
    }

    QApt::Package::State action = checked ? QApt::Package::ToInstall : QApt::Package::ToKeep;

    if (list.size() > 1) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }

    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    m_backend->markPackages(list, action);

    // Check for removals
    auto changes = m_backend->stateChanges(m_oldCacheState, list);

    checkChanges(changes);

    // Update check values
    m_updateModel->updateCheckStates();

    QApplication::restoreOverrideCursor();
}

void UpdaterWidget::checkChanges(const QHash<QApt::Package::State, QApt::PackageList> &changes)
{
    if (changes.isEmpty()) {
        return;
    }

    ChangesDialog *dialog = new ChangesDialog(this, changes);
    int res = dialog->exec();

    if (res != QDialog::Accepted)
        m_backend->restoreCacheState(m_oldCacheState);
}

void UpdaterWidget::selectionChanged(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    QModelIndexList indexes = selected.indexes();
    QApt::Package *package = 0;

    if (!indexes.isEmpty()) {
        package = retrievePackage(m_updateModel->itemFromIndex(indexes.first())->app());
    }

    emit selectedPackageChanged(package);
}

void UpdaterWidget::checkAllMarked()
{
    QApt::PackageList upgradeable = m_backend->upgradeablePackages();
    int markedCount = m_backend->packageCount(QApt::Package::ToUpgrade);

    m_upgradesWidget->setVisible(markedCount < upgradeable.count());
}

void UpdaterWidget::markAllPackagesForUpgrade()
{
    QApt::PackageList upgradeable = m_backend->upgradeablePackages();

    // Mark dist upgrade
    m_oldCacheState = m_backend->currentCacheState();
    m_backend->saveCacheState();
    m_backend->markPackagesForDistUpgrade();

    m_updateModel->updateCheckStates();
    checkChanges(m_backend->stateChanges(m_oldCacheState, upgradeable));

    m_upgradesWidget->hide();
    emit modelPopulated();
}

void UpdaterWidget::checkUpToDate()
{
    if(m_backend->upgradeablePackages().isEmpty()) {
        setCurrentIndex(1);
        QDateTime lastUpdate = m_backend->timeCacheLastUpdated();

        // Unknown time since last update
        if (!lastUpdate.isValid()) {
            m_updateStatusIcon->setPixmap(KIcon("security-medium").pixmap(128, 128));
            m_notifyTitle->setText(i18nc("@info",
                                         "It is unknown when the last check for updates was."));
            m_notifyDesc->setText(i18nc("@info", "Please click <interface>Check for Updates</interface> "
                                        "to check."));
            return;
        }

        qint64 msecSinceUpdate = lastUpdate.msecsTo(QDateTime::currentDateTime());
        qint64 day = 1000 * 60 * 60 * 24;
        qint64 week = 1000 * 60 * 60 * 24 * 7;

        if (msecSinceUpdate < day) {
            m_updateStatusIcon->setPixmap(KIcon("security-high").pixmap(128, 128));
            m_notifyTitle->setText(i18nc("@info", "The software on this computer is up to date."));
            m_notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KGlobal::locale()->prettyFormatDuration(msecSinceUpdate)));
        } else if (msecSinceUpdate < week) {
            m_updateStatusIcon->setPixmap(KIcon("security-medium").pixmap(128, 128));
            m_notifyTitle->setText(i18nc("@info", "No updates are available."));
            m_notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KGlobal::locale()->prettyFormatDuration(msecSinceUpdate)));
        } else {
            m_updateStatusIcon->setPixmap(KIcon("security-low").pixmap(128, 128));
            m_notifyTitle->setText("The last check for updates was over a week ago.");
            m_notifyDesc->setText(i18nc("@info", "Please click <interface>Check for Updates</interface> "
                                        "to check."));
        }
    }
}

QApt::Package* UpdaterWidget::retrievePackage(AbstractResource* res)
{
    return res ? m_backend->package(res->packageName()) : 0;
}
