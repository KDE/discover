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
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QAction>
#include <QPushButton>
#include <QDebug>

// KDE includes
#include <KLocalizedString>
#include <KMessageBox>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KMessageWidget>
#include <KFormat>
#include <KIconLoader>

// DiscoverCommon includes
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/ResourcesModel.h>
#include <UpdateModel/UpdateModel.h>
#include <UpdateModel/UpdateItem.h>

// Own includes
#include "UpdateDelegate.h"
#include "ChangelogWidget.h"
#include "ui_UpdaterWidgetNoUpdates.h"

UpdaterWidget::UpdaterWidget(ResourcesUpdatesModel* updates, QWidget *parent) :
    QStackedWidget(parent), m_updatesBackends(updates)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_updateModel = new UpdateModel(this);
    m_updateModel->setBackend(updates);

    // First page (update view)
    QWidget *page1 = new QWidget(this);
    QVBoxLayout * page1Layout = new QVBoxLayout(page1);
    page1Layout->setMargin(0);
    page1Layout->setSpacing(0);
    page1->setLayout(page1Layout);

    QString text = i18nc("@label", "<em>Some packages were not marked for update.</em><p/>"
                            "The update of these packages need some others to be installed or removed.<p/>"
                            "Do you want to update those too?");
    QAction* action = new QAction(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")), i18n("Mark All"), this);
    connect(action, &QAction::triggered, this, &UpdaterWidget::markAllPackagesForUpgrade);
    m_markallWidget = new KMessageWidget(this);
    m_markallWidget->setText(text);
    m_markallWidget->addAction(action);
    m_markallWidget->setCloseButtonVisible(true);
    m_markallWidget->setVisible(false);

    m_changelogWidget = new ChangelogWidget(this);
    m_changelogWidget->hide();
    connect(this, &UpdaterWidget::selectedResourceChanged, m_changelogWidget, &ChangelogWidget::setResource);

    QLabel* descriptionLabel = new QLabel(this);
    descriptionLabel->setText(i18n("<p><b>New software is available for your computer</b></p><p>Click 'Install Updates' to keep your system up-to-date and safe</p>"));
    descriptionLabel->setMinimumHeight(descriptionLabel->fontMetrics().height()*4);

    m_updateView = new QTreeView(page1);
    m_updateView->setAlternatingRowColors(true);
    m_updateView->setModel(m_updateModel);
    m_updateView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_updateView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_updateView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_updateView->header()->setStretchLastSection(false);
    connect(m_updateView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &UpdaterWidget::selectionChanged);

    page1Layout->addWidget(m_markallWidget);
    page1Layout->addWidget(descriptionLabel);
    page1Layout->addWidget(m_updateView);
    page1Layout->addWidget(m_changelogWidget);

    m_busyWidget = new KPixmapSequenceOverlayPainter(page1);
    KPixmapSequence seq = KIconLoader::global()->loadPixmapSequence(QStringLiteral("process-working"), KIconLoader::SizeSmallMedium);

    m_busyWidget->setSequence(seq);
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

    ResourcesModel* resourcesModel = ResourcesModel::global();
    connect(resourcesModel, &ResourcesModel::fetchingChanged, this, &UpdaterWidget::activityChanged);
    connect(resourcesModel, &ResourcesModel::updatesCountChanged, this, &UpdaterWidget::activityChanged);
    connect(m_updatesBackends, &ResourcesUpdatesModel::progressingChanged, this, &UpdaterWidget::activityChanged);
}

UpdaterWidget::~UpdaterWidget()
{
    delete m_ui;
}

void UpdaterWidget::activityChanged()
{
    if(ResourcesModel::global()->isFetching()) {
        m_busyWidget->start();
        setEnabled(false);
        setCurrentIndex(0);
    } else if(m_updatesBackends->isProgressing()) {
        setCurrentIndex(-1);
        m_changelogWidget->hide();
        m_busyWidget->start();
        setEnabled(false);
    } else {
        populateUpdateModel();
        setEnabled(true);
    }
}

void UpdaterWidget::populateUpdateModel()
{
    m_busyWidget->stop();
    QApplication::restoreOverrideCursor();
    setEnabled(true);
    checkUpToDate();
    if (!m_updatesBackends->hasUpdates()) {
        return;
    }

    m_updateView->expand(m_updateModel->index(0,0)); // Expand apps category
    m_updateView->resizeColumnToContents(0);
    m_updateView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    checkAllMarked();
    emit modelPopulated();
}

void UpdaterWidget::selectionChanged(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    QModelIndexList indexes = selected.indexes();
    AbstractResource *res = nullptr;

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
    emit modelPopulated();
}

void UpdaterWidget::checkUpToDate()
{
    const QDateTime lastUpdate = m_updatesBackends->lastUpdate();
    const qint64 msecSinceUpdate = lastUpdate.msecsTo(QDateTime::currentDateTime());
    const qint64 day = 1000 * 60 * 60 * 24;
    const qint64 week = 1000 * 60 * 60 * 24 * 7;

    if((!m_updatesBackends->hasUpdates() && !ResourcesModel::global()->isFetching()) || msecSinceUpdate > week) {
        setCurrentIndex(1);

        // Unknown time since last update
        if (!lastUpdate.isValid()) {
            m_ui->updateStatusIcon->setPixmap(QIcon::fromTheme(QStringLiteral("security-medium")).pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info",
                                         "It is unknown when the last check for updates was."));
            m_ui->notifyDesc->setText(i18nc("@info", "Please click <em>Check for Updates</em> "
                                        "to check."));
            return;
        }

        if (msecSinceUpdate < day) {
            m_ui->updateStatusIcon->setPixmap(QIcon::fromTheme(QStringLiteral("security-high")).pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info", "The software on this computer is up to date."));
            m_ui->notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KFormat().formatDecimalDuration(msecSinceUpdate, 0)));
        } else if (msecSinceUpdate < week) {
            m_ui->updateStatusIcon->setPixmap(QIcon::fromTheme(QStringLiteral("security-medium")).pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info", "No updates are available."));
            m_ui->notifyDesc->setText(i18nc("@info", "Last checked %1 ago.",
                                        KFormat().formatDecimalDuration(msecSinceUpdate, 0)));
        } else {
            m_ui->updateStatusIcon->setPixmap(QIcon::fromTheme(QStringLiteral("security-low")).pixmap(128, 128));
            m_ui->notifyTitle->setText(i18nc("@info", "The last check for updates was over a week ago."));
            m_ui->notifyDesc->setText(i18nc("@info", "Please click <em>Check for Updates</em> "
                                        "to check."));
        }
    } else
        setCurrentIndex(0);
}
