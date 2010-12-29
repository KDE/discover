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
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KVBox>

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

    m_busyWidget = new KPixmapSequenceOverlayPainter(this);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_packageView->viewport());

    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_busyWidget->start();

    connect(m_packageView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(packageActivated(const QModelIndex &)));
    connect(m_packageView, SIGNAL(currentPackageChanged(const QModelIndex &)),
            this, SLOT(packageActivated(const QModelIndex &)));
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

#include "PackageWidget.moc"
