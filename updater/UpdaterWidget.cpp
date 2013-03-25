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
#include <QDateTime>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <qaction.h>
#include <QPushButton>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KDebug>
#include <KMessageWidget>

// Libmuon includes
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesUpdatesModel.h>
// #include <resources/ResourcesModel.h>

// Own includes
#include "UpdateModel/UpdateModel.h"
#include "UpdateModel/UpdateItem.h"
#include "UpdateModel/UpdateDelegate.h"
#include "ChangelogWidget.h"
#include "ui_UpdaterWidgetNoUpdates.h"

UpdaterWidget::UpdaterWidget(QWidget *parent) :
    QStackedWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_updateModel = new UpdateModel(this);
    connect(m_updateModel, SIGNAL(checkApps(QList<AbstractResource*>,bool)),
            this, SLOT(checkApps(QList<AbstractResource*>,bool)));

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
    m_markallWidget = new KMessageWidget(this);
    m_markallWidget->setText(text);
    m_markallWidget->addAction(action);
    m_markallWidget->setCloseButtonVisible(true);
    m_markallWidget->setVisible(false);

    ChangelogWidget* changelogWidget = new ChangelogWidget(this);
    changelogWidget->hide();
    connect(this, SIGNAL(selectedResourceChanged(AbstractResource*)),
            changelogWidget, SLOT(setResource(AbstractResource*)));

    m_descriptionLabel = new QLabel(this);
    connect(m_descriptionLabel, SIGNAL(linkActivated(QString)), SLOT(toggleUpdateVisibility()));

    m_updateView = new QTreeView(page1);
    m_updateView->setVisible(false);
    m_updateView->setAlternatingRowColors(true);
    m_updateView->setModel(m_updateModel);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_updateView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_updateView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
    m_updateView->header()->setStretchLastSection(false);
    connect(m_updateView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));

    page1Layout->addWidget(m_markallWidget);
    page1Layout->addWidget(m_descriptionLabel);
    page1Layout->addWidget(m_updateView);
    page1Layout->addWidget(changelogWidget);

    m_busyWidget = new KPixmapSequenceOverlayPainter(page1);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_updateView->viewport());

    UpdateDelegate *delegate = new UpdateDelegate(m_updateView);
    m_updateView->setItemDelegate(delegate);

    // Second page (If no updates, show when last check or warn about lack of updates)
    QWidget *page2 = new QWidget(this);
    m_ui = new Ui::UpdaterWidgetNoUpdates;
    m_ui->setupUi(page2);

    addWidget(page1);
    addWidget(page2);
    setCurrentWidget(page1);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_busyWidget->start();
    initializeDescription();
}

UpdaterWidget::~UpdaterWidget()
{
    delete m_ui;
}

void UpdaterWidget::setBackend(ResourcesUpdatesModel *updates)
{
    m_updatesBackends = updates;
    connect(m_updatesBackends, SIGNAL(progressingChanged()), SLOT(activityChanged()));

    populateUpdateModel();
    setEnabled(true);
}

void UpdaterWidget::activityChanged()
{
    if(m_updatesBackends->isProgressing()) {
        m_busyWidget->start();
        setEnabled(false);
        setCurrentIndex(-1);
    } else {
        populateUpdateModel();
    }
}

void UpdaterWidget::populateUpdateModel()
{
    m_busyWidget->stop();
    QApplication::restoreOverrideCursor();
    setEnabled(true);
    if (!m_updatesBackends->hasUpdates()) {
        checkUpToDate();
        return;
    }
    m_updatesBackends->prepare();
    m_updateModel->setResources(m_updatesBackends->toUpdate());

    m_updateView->expand(m_updateModel->index(0,0)); // Expand apps category
    m_updateView->resizeColumnToContents(0);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);

    checkAllMarked();
    checkUpToDate();
}

void UpdaterWidget::checkApps(const QList<AbstractResource*>& apps, bool checked)
{
    if (apps.size() > 1) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    if(checked)
        m_updatesBackends->addResources(apps);
    else
        m_updatesBackends->removeResources(apps);
    QApplication::restoreOverrideCursor();
}

void UpdaterWidget::selectionChanged(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    QModelIndexList indexes = selected.indexes();
    AbstractResource *res = 0;

    if (!indexes.isEmpty()) {
        res = m_updateModel->itemFromIndex(indexes.first())->app();
    }

    emit selectedResourceChanged(res);
}

void UpdaterWidget::checkAllMarked()
{
    m_markallWidget->setVisible(!m_updatesBackends->isAllMarked());
}

void UpdaterWidget::markAllPackagesForUpgrade()
{
    m_updatesBackends->prepare();
    m_markallWidget->hide();
}

void UpdaterWidget::checkUpToDate()
{
    if(!m_updatesBackends->hasUpdates()) {
        setCurrentIndex(1);
        QDateTime lastUpdate = m_updatesBackends->lastUpdate();

        // Unknown time since last update
        if (!lastUpdate.isValid()) {
            m_ui->updateStatusIcon->setPixmap(KIcon("security-medium").pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info",
                                         "It is unknown when the last check for updates was."));
            m_ui->notifyDesc->setText(i18nc("@info", "Please click <interface>Check for Updates</interface> "
                                        "to check."));
            return;
        }

        qint64 msecSinceUpdate = lastUpdate.msecsTo(QDateTime::currentDateTime());
        qint64 day = 1000 * 60 * 60 * 24;
        qint64 week = 1000 * 60 * 60 * 24 * 7;

        if (msecSinceUpdate < day) {
            m_ui->updateStatusIcon->setPixmap(KIcon("security-high").pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info", "The software on this computer is up to date."));
            m_ui->notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KGlobal::locale()->prettyFormatDuration(msecSinceUpdate)));
        } else if (msecSinceUpdate < week) {
            m_ui->updateStatusIcon->setPixmap(KIcon("security-medium").pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info", "No updates are available."));
            m_ui->notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KGlobal::locale()->prettyFormatDuration(msecSinceUpdate)));
        } else {
            m_ui->updateStatusIcon->setPixmap(KIcon("security-low").pixmap(128, 128));
            m_ui->notifyTitle->setText("The last check for updates was over a week ago.");
            m_ui->notifyDesc->setText(i18nc("@info", "Please click <interface>Check for Updates</interface> "
                                        "to check."));
        }
    }
}

void UpdaterWidget::toggleUpdateVisibility()
{
    m_updateView->setVisible(!m_updateView->isVisible());
}

void UpdaterWidget::initializeDescription()
{
    if(m_updateView->isVisible())
        m_descriptionLabel->setText(i18n("123 GiB of updates. <a href='fuuu'>Hide package list</a>"));
    else
        m_descriptionLabel->setText(i18n("123 GiB of updates. <a href='fuuu'>Show package list</a>"));
}
