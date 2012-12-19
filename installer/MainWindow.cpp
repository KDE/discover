/***************************************************************************
 *   Copyright © 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#include "MainWindow.h"

// Qt includes
#include <QStandardItemModel>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <QtGui/QAbstractItemView>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>

// KDE includes
#include <KAction>
#include <KActionCollection>
#include <KIcon>
#include <KMessageBox>
#include <KMessageWidget>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KService>
#include <KToolInvocation>

// LibQApt includes
#include <LibQApt/Backend>

//libmuonapt includes
#include <HistoryView/HistoryView.h>
#include "../libmuonapt/QAptActions.h"

// Libmuon includes
#include <resources/ResourcesModel.h>
#include <MuonBackendsFactory.h>

// Own includes
#include "ApplicationLauncher.h"
#include "ResourceView/ResourceListView.h"
#include "AvailableView.h"
#include "ProgressView.h"
#include "ViewSwitcher.h"
#include "LaunchListModel.h"
#include "MuonInstallerSettings.h"

enum ViewModelRole {
    /// A role for storing ViewType
    ViewTypeRole = Qt::UserRole + 1,
    /// A role for storing origin filter data
    OriginFilterRole = Qt::UserRole + 2,
    /// A role for storing state filter data
    StateFilterRole = Qt::UserRole + 3
};

enum ViewType {
    /// An invalid value
    InvalidView = 0,
    /// A simple ApplicationView that is filterable by status or origin
    AppView,
    /// An ApplicationView that has a Categorical homepage
    CatView,
    /// A CategoryView showing subcategories
    SubCatView,
    /// A view for showing history
    History,
    /// A view for showing in-progress transactions
    Progress
};

MainWindow::MainWindow()
    : MuonMainWindow()
    , m_appBackend(nullptr)
    , m_launches(nullptr)
    , m_launcherMessage(nullptr)
    , m_appLauncher(nullptr)
    , m_progressItem(nullptr)
    , m_transactionCount(0)
{
    initGUI();
    QTimer::singleShot(10, this, SLOT(initObject()));
}

MainWindow::~MainWindow()
{
    MuonInstallerSettings::self()->writeConfig();
}

void MainWindow::initGUI()
{
    m_mainWidget = new QSplitter(this);
    m_mainWidget->setOrientation(Qt::Horizontal);
    connect(m_mainWidget, SIGNAL(splitterMoved(int,int)), this, SLOT(saveSplitterSizes()));
    setCentralWidget(m_mainWidget);

    // Set up the navigational sidebar on the right
    m_viewSwitcher = new ViewSwitcher(this);
    connect(m_viewSwitcher, SIGNAL(activated(QModelIndex)),
           this, SLOT(changeView(QModelIndex)));
    m_mainWidget->addWidget(m_viewSwitcher);

    // Set up the main pane
    KVBox *leftWidget = new KVBox(this);
    m_launcherMessage = new KMessageWidget(leftWidget);
    m_launcherMessage->hide();
    m_launcherMessage->setMessageType(KMessageWidget::Positive);

    m_viewStack = new QStackedWidget(leftWidget);
    m_viewStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Busy widget
    m_busyWidget = new QWidget(m_viewStack);
    KPixmapSequenceOverlayPainter *busyWidget = new KPixmapSequenceOverlayPainter(m_busyWidget);
    busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    busyWidget->setWidget(m_busyWidget);
    busyWidget->start();

    m_viewStack->addWidget(m_busyWidget);
    m_viewStack->setCurrentWidget(m_busyWidget);

    m_mainWidget->addWidget(leftWidget);
    loadSplitterSizes();

    m_viewModel = new QStandardItemModel(this);
    m_viewSwitcher->setModel(m_viewModel);

    QAptActions *actions = QAptActions::self();
    actions->setMainWindow(this);

    setupActions();
    setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::initObject()
{
    ResourcesModel *resourcesModel = ResourcesModel::global();
    connect(resourcesModel, SIGNAL(transactionAdded(Transaction*)),
            this, SLOT(transactionAdded()));
    connect(resourcesModel, SIGNAL(transactionRemoved(Transaction*)),
            this, SLOT(transactionRemoved()));

    m_launches = new LaunchListModel(this);

    MuonBackendsFactory f;
    QList<AbstractResourcesBackend*> backends = f.allBackends();

    //TODO: should add the appBackend here too
    for (AbstractResourcesBackend *backend : backends) {
        resourcesModel->addResourcesBackend(backend);
        backend->integrateMainWindow(this);
        
        if(backend->metaObject()->className()==QLatin1String("ApplicationBackend")) {
            m_appBackend = backend;
            connect(m_appBackend, SIGNAL(backendReady()),
                    this, SLOT(populateViews()));
            connect(m_appBackend, SIGNAL(reloadStarted()), //TODO: use ResourcesModel signals
                    this, SLOT(removeProgressItem()));
            connect(m_appBackend, SIGNAL(reloadFinished()),
                    this, SLOT(showLauncherMessage()));
            connect(m_appBackend, SIGNAL(startingFirstTransaction()),
                    this, SLOT(addProgressItem()));
            connect(m_appBackend, SIGNAL(sourcesEditorFinished()),
                    this, SLOT(sourcesEditorFinished()));
        }
    }
}

void MainWindow::loadSplitterSizes()
{
    QList<int> sizes = MuonInstallerSettings::self()->splitterSizes();

    if (sizes.isEmpty()) {
        sizes << 115 << (this->width() - 115);
    }
    m_mainWidget->setSizes(sizes);
}

void MainWindow::saveSplitterSizes()
{
    MuonInstallerSettings::self()->setSplitterSizes(m_mainWidget->sizes());
    MuonInstallerSettings::self()->writeConfig();
}

void MainWindow::clearViews()
{
    setCanExit(false); // APT is reloading at this point
    foreach (QWidget *widget, m_viewHash) {
        delete widget;
    }
    m_viewHash.clear();
    m_viewModel->clear();
}

QStandardItem* createOriginItem(const QString& originName, const QString& originLabel)
{
    // We must spread the word of Origin. Hallowed are the Ori! ;P
    QStandardItem *viewItem = new QStandardItem;
    viewItem->setEditable(false);
    viewItem->setText(originLabel);
    viewItem->setData(originName, OriginFilterRole);
    viewItem->setData(AppView, ViewTypeRole);

    if (originName == "Ubuntu") {
        viewItem->setText(i18nc("@item:inlistbox", "Provided by Kubuntu"));
        viewItem->setIcon(KIcon("ubuntu-logo"));
    }

    if (originName == "Debian") {
        viewItem->setText(i18nc("@item:inlistbox", "Provided by Debian"));
        viewItem->setIcon(KIcon("emblem-debian"));
    }

    if (originName == "Canonical") {
        viewItem->setText(i18nc("@item:inlistbox The name of the repository provided by Canonical, Ltd. ",
                                "Canonical Partners"));
        viewItem->setIcon(KIcon("partner"));
    }

    if (originName.startsWith(QLatin1String("LP-PPA"))) {
        viewItem->setIcon(KIcon("user-identity"));

        if (originName == QLatin1String("LP-PPA-app-review-board")) {
            viewItem->setText(i18nc("@item:inlistbox An independent software source",
                                    "Independent"));
            viewItem->setIcon(KIcon("system-users"));
        } else
            viewItem->setIcon(KIcon("user-identity"));
    }
    return viewItem;
}

bool repositoryNameLessThan(const QString& a, const QString& b)
{
    static QStringList prioritary(QStringList() << "Debian" << "Ubuntu" << "Canonical" << "LP-PPA-app-review-board");
    int idxA = prioritary.indexOf(a), idxB = prioritary.indexOf(b);
    if(idxA == idxB)
        return a<b;
    else if((idxB == -1) ^ (idxA == -1))
        return idxB == -1;
    else
        return idxA<idxB;
}

QPair<QStringList, QStringList> fetchOrigins()
{
    QSet<QString> originSet, instOriginSet;
    
    ResourcesModel *resourcesModel = ResourcesModel::global();
    for(int i=0; i<resourcesModel->rowCount(); i++) {
        AbstractResource* app = resourcesModel->resourceAt(i);
        if (app->isInstalled())
            instOriginSet << app->origin();
        else
            originSet << app->origin();
    }

    originSet.remove(QString());
    instOriginSet.remove(QString());
    originSet += instOriginSet;
    
    QStringList originList=originSet.toList(), instOriginList=instOriginSet.toList();
    qSort(originList.begin(), originList.end(), repositoryNameLessThan);
    qSort(instOriginList.begin(), instOriginList.end(), repositoryNameLessThan);
    return qMakePair(originList, instOriginList);
}

void MainWindow::populateViews()
{
    QStandardItem *availableItem = new QStandardItem;
    availableItem->setEditable(false);
    availableItem->setIcon(KIcon("applications-other").pixmap(32,32));
    availableItem->setText(i18nc("@item:inlistbox Parent item for available software", "Get Software"));
    availableItem->setData(CatView, ViewTypeRole);

    QStandardItem *installedItem = new QStandardItem;
    installedItem->setEditable(false);
    installedItem->setIcon(KIcon("computer"));
    installedItem->setText(i18nc("@item:inlistbox Parent item for installed software", "Installed Software"));
    installedItem->setData(AppView, ViewTypeRole);
    installedItem->setData(AbstractResource::State::Installed, StateFilterRole);
    
    QPair< QStringList, QStringList > origins = fetchOrigins();
    QStringList originNames = origins.first;
    QApt::Backend* backend = qobject_cast<QApt::Backend*>(m_appBackend->property("backend").value<QObject*>());
    foreach(const QString &originName, originNames) {
        availableItem->appendRow(createOriginItem(originName, backend->originLabel(originName)));
    }

    QStringList instOriginNames = origins.second;
    foreach(const QString & originName, instOriginNames) {
        QStandardItem* viewItem = createOriginItem(originName, backend->originLabel(originName));

        viewItem->setData(AbstractResource::State::Installed, StateFilterRole);
        installedItem->appendRow(viewItem);
    }

    QStandardItem *historyItem = new QStandardItem;
    historyItem->setEditable(false);
    historyItem->setIcon(KIcon("view-history").pixmap(32,32));
    historyItem->setText(i18nc("@item:inlistbox Item for showing the history view", "History"));
    historyItem->setData(History, ViewTypeRole);

    m_viewModel->appendRow(availableItem);
    m_viewModel->appendRow(installedItem);
    m_viewModel->appendRow(historyItem);
    selectFirstRow(m_viewSwitcher);
}

void MainWindow::changeView(const QModelIndex &index)
{
    QWidget *view = m_viewHash.value(index);

    // Create new widget if not already created
    if (!view) {
        switch (index.data(ViewTypeRole).toInt()) {
        case AppView: {
            QString originFilter = index.data(OriginFilterRole).toString();
            AbstractResource::State stateFilter = (AbstractResource::State)index.data(StateFilterRole).toInt();

            view = new ResourceListView(this, index);
            ResourceListView *appView = static_cast<ResourceListView *>(view);
            appView->setStateFilter(stateFilter);
            appView->setOriginFilter(originFilter);
            appView->setCanShowTechnical(true);

            if (originFilter != QLatin1String("Ubuntu") && originFilter != QLatin1String("Debian"))
                appView->setShouldShowTechnical(true);
        }
        break;
        case CatView:
            view = new AvailableView(this);
            break;
        case History:
            view = new HistoryView(this);
            break;
        case Progress:
            view = new ProgressView(this);
        case InvalidView:
        default:
            break;
        }

        m_viewStack->addWidget(view);
    }

    m_viewStack->addWidget(view);
    m_viewStack->setCurrentWidget(view);
    m_viewStack->removeWidget(m_busyWidget);

    delete m_busyWidget;
    m_busyWidget = nullptr;

    m_viewHash[index] = view;
}

void MainWindow::selectFirstRow(const QAbstractItemView *itemView)
{
    QModelIndex firstRow = itemView->model()->index(0, 0);
    itemView->selectionModel()->select(firstRow, QItemSelectionModel::Select);
    changeView(firstRow);
}

void MainWindow::runSourcesEditor()
{
    // Let QApt Batch handle the update GUI
    // FIXME?
    //MuonMainWindow::runSourcesEditor(true);
}

void MainWindow::sourcesEditorFinished()
{
    clearViews();
    populateViews();
    find(effectiveWinId())->setEnabled(true);
}

void MainWindow::showLauncherMessage()
{
    clearMessageActions();

    if (m_launches->rowCount()==1) {
        KService::Ptr service = m_launches->serviceAt(0);
        QString name = service->genericName().isEmpty() ?
                       service->property("Name").toString() :
                       service->property("Name").toString() % QLatin1Literal(" - ") % service->genericName();
        m_launcherMessage->setText(i18nc("@info", "%1 was successfully installed.", name));

        KAction *launchAction = new KAction(KIcon(service->icon()), i18nc("@action", "Start"), this);
        connect(launchAction, SIGNAL(activated()), this, SLOT(launchSingleApp()));

        m_launcherMessage->addAction(launchAction);
        m_launcherMessage->animatedShow();
    } else if (m_launches->rowCount() > 1) {
        m_launcherMessage->setText(i18nc("@info", "Applications successfully installed."));
        KAction *launchAction = new KAction(i18nc("@action", "Run New Applications..."), this);
        connect(launchAction, SIGNAL(activated()), this, SLOT(showAppLauncher()));
        m_launcherMessage->addAction(launchAction);
        m_launcherMessage->animatedShow();
    }
}

void MainWindow::launchSingleApp()
{
    m_launches->invokeApplication(0);
    m_launcherMessage->animatedHide();
    m_launcherMessage->removeAction(m_launcherMessage->actions().first());
}

void MainWindow::showAppLauncher()
{
    if (!m_appLauncher && !m_launches->rowCount()==0) {
        m_appLauncher = new ApplicationLauncher(m_launches);
        connect(m_appLauncher, SIGNAL(destroyed(QObject*)),
            this, SLOT(onAppLauncherClosed()));
        connect(m_appLauncher, SIGNAL(finished(int)),
            this, SLOT(onAppLauncherClosed()));
        m_appLauncher->setWindowTitle(i18nc("@title:window", "Installation Complete"));
        m_appLauncher->show();
    }
    m_launcherMessage->animatedHide();
}

void MainWindow::onAppLauncherClosed()
{
    m_appLauncher = 0;
}

void MainWindow::clearMessageActions()
{
    foreach (QAction *action, m_launcherMessage->actions()) {
        m_launcherMessage->removeAction(action);
    }
}

void MainWindow::transactionAdded()
{
    m_transactionCount++;
}

void MainWindow::transactionRemoved()
{
    if (m_transactionCount)
        m_transactionCount--;

    if (!m_transactionCount)
        removeProgressItem();
}

void MainWindow::addProgressItem()
{
    QStandardItem *parentItem = m_viewModel->invisibleRootItem();

    if (m_progressItem)
        return;

    m_progressItem = new QStandardItem;
    m_progressItem->setEditable(false);
    m_progressItem->setIcon(KIcon("download").pixmap(32,32));
    m_progressItem->setText(i18nc("@item:inlistbox Item for showing the progress view", "In Progress"));
    m_progressItem->setData(Progress, ViewTypeRole);
    parentItem->appendRow(m_progressItem);

    m_viewHash[m_progressItem->index()] = 0;
}

void MainWindow::removeProgressItem()
{
    if (!m_progressItem)
        return;

    QObject *progressView = m_viewHash.take(m_progressItem->index());
    if (progressView)
            progressView->deleteLater();

    m_viewModel->removeRow(m_progressItem->row());
    m_progressItem = nullptr;
}
