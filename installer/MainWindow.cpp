/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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
#include <KMessageWidget>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KService>
#include <KToolInvocation>

// LibQApt includes
#include <LibQApt/Backend>

// Libmuon includes
#include <Application.h>
#include <ApplicationBackend.h>
#include <HistoryView/HistoryView.h>
#include <resources/ResourcesModel.h>

// Own includes
#include "ApplicationLauncher.h"
#include "ApplicationView/ApplicationListView.h"
#include "AvailableView.h"
#include "ProgressView.h"
#include "ViewSwitcher.h"
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

    setupActions();
    setupGUI((StandardWindowOption)(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
}

void MainWindow::initObject()
{
    ResourcesModel *resourcesModel = ResourcesModel::global();
    connect(resourcesModel, SIGNAL(transactionAdded(Transaction*)),
            this, SLOT(transactionAdded()));
    connect(resourcesModel, SIGNAL(transactionRemoved(Transaction*)),
            this, SLOT(transactionRemoved()));

    // Create APT backend
    m_appBackend = new ApplicationBackend(this);
    connect(this, SIGNAL(backendReady(QApt::Backend*)),
            m_appBackend, SLOT(setBackend(QApt::Backend*)));
    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent,Transaction*)),
            this, SLOT(workerEvent(QApt::WorkerEvent)));
    connect(m_appBackend, SIGNAL(errorSignal(QApt::ErrorCode,QVariantMap)),
            this, SLOT(errorOccurred(QApt::ErrorCode,QVariantMap)));
    connect(m_appBackend, SIGNAL(backendReady()),
            this, SLOT(populateViews()));
    connect(m_appBackend, SIGNAL(reloadStarted()),
            this, SLOT(removeProgressItem()));
    connect(m_appBackend, SIGNAL(reloadFinished()),
            this, SLOT(showLauncherMessage()));
    connect(m_appBackend, SIGNAL(startingFirstTransaction()),
            this, SLOT(addProgressItem()));

    MuonMainWindow::initObject();
    // Our modified ApplicationBackend provides us events in a way that
    // makes queuing things while committing possible
    disconnect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)),
               this, SLOT(workerEvent(QApt::WorkerEvent)));
    disconnect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode,QVariantMap)),
               this, SLOT(errorOccurred(QApt::ErrorCode,QVariantMap)));

    // Other backends
//    QList<AbstractResourcesBackend*> backends;

//    for (AbstractResourcesBackend *backend : backends) {
//        resourcesModel->addResourcesBackend(backend);
//    }

    setActionsEnabled();
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

void MainWindow::setupActions()
{
    MuonMainWindow::setupActions();

    m_loadSelectionsAction = actionCollection()->addAction("open_markings");
    m_loadSelectionsAction->setIcon(KIcon("document-open"));
    m_loadSelectionsAction->setText(i18nc("@action", "Read Markings..."));
    connect(m_loadSelectionsAction, SIGNAL(triggered()), this, SLOT(loadSelections()));

    m_saveSelectionsAction = actionCollection()->addAction("save_markings");
    m_saveSelectionsAction->setIcon(KIcon("document-save-as"));
    m_saveSelectionsAction->setText(i18nc("@action", "Save Markings As..."));
    connect(m_saveSelectionsAction, SIGNAL(triggered()), this, SLOT(saveSelections()));
}

void MainWindow::setActionsEnabled(bool enabled)
{
    MuonMainWindow::setActionsEnabled(enabled);
    if (!enabled) {
        return;
    }

    m_loadSelectionsAction->setEnabled(true);
    m_saveSelectionsAction->setEnabled(m_backend->areChangesMarked());
}

void MainWindow::clearViews()
{
    m_canExit = false; // APT is reloading at this point
    foreach (QWidget *widget, m_viewHash) {
        delete widget;
    }
    m_viewHash.clear();
    m_viewModel->clear();
}

void MainWindow::checkForUpdates()
{
    m_backend->updateCache();
}

void MainWindow::workerEvent(QApt::WorkerEvent event)
{
    MuonMainWindow::workerEvent(event);
 
    switch (event) {
    case QApt::CommitChangesFinished:
        if (m_warningStack.size() > 0) {
            showQueuedWarnings();
            m_warningStack.clear();
        }
        if (m_errorStack.size() > 0) {
            showQueuedErrors();
            m_errorStack.clear();
        }
        break;
    case QApt::InvalidEvent:
    default:
        break;
    }
}

void MainWindow::populateViews()
{
    ResourcesModel *resourcesModel = ResourcesModel::global();
    resourcesModel->addResourcesBackend(m_appBackend);
    m_canExit = true; // APT is done reloading at this point
    QStringList originNames = m_appBackend->appOrigins().toList();
    QStringList originLabels;
    foreach (const QString &originName, originNames) {
        originLabels << m_backend->originLabel(originName);
    }

    if (originNames.contains("Ubuntu")) {
        int index = originNames.indexOf("Ubuntu");
        originNames.move(index, 0); // Move to front of the list

        if (originNames.contains("Canonical")) {
            int index = originNames.indexOf("Canonical");
            if (originNames.size() >= 2)
                originNames.move(index, 1); // Move to 2nd spot
        }

        if (originNames.contains("LP-PPA-app-review-board")) {
            int index = originNames.indexOf("LP-PPA-app-review-board");
            if (originNames.size() >= 3)
                originNames.move(index, 2); // Move to third spot
        }
    }

    if (originNames.contains("Debian")) {
        int index = originNames.indexOf("Debian");
        originNames.move(index, 0); // Move to front of the list
    }

    QStandardItem *parentItem = m_viewModel->invisibleRootItem();

    QStandardItem *availableItem = new QStandardItem;
    availableItem->setEditable(false);
    availableItem->setIcon(KIcon("applications-other").pixmap(32,32));
    availableItem->setText(i18nc("@item:inlistbox Parent item for available software", "Get Software"));
    availableItem->setData(CatView, ViewTypeRole);
    parentItem->appendRow(availableItem);
    m_viewHash[availableItem->index()] = 0;

    QStandardItem *installedItem = new QStandardItem;
    installedItem->setEditable(false);
    installedItem->setIcon(KIcon("computer"));
    installedItem->setText(i18nc("@item:inlistbox Parent item for installed software", "Installed Software"));
    installedItem->setData(AppView, ViewTypeRole);
    installedItem->setData(AbstractResource::State::Installed, StateFilterRole);
    parentItem->appendRow(installedItem);
    m_viewHash[installedItem->index()] = 0;

    parentItem = availableItem;
    foreach(const QString &originName, originNames) {
        QString originLabel = m_backend->originLabel(originName);
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
            }
        }

        availableItem->appendRow(viewItem);
        m_viewHash[viewItem->index()] = 0;
    }

    QStringList instOriginNames = m_appBackend->installedAppOrigins().toList();
    QStringList instOriginLabels;
    foreach (const QString &originName, instOriginNames) {
        instOriginLabels << m_backend->originLabel(originName);
    }

    if (instOriginNames.contains("Ubuntu")) {
        int index = instOriginNames.indexOf("Ubuntu");
        instOriginNames.move(index, 0); // Move to front of the list

        if (instOriginNames.contains("Canonical")) {
            int index = instOriginNames.indexOf("Canonical");
            if (originNames.size() >= 2)
                instOriginNames.move(index, 1); // Move to 2nd spot
        }

        if (instOriginNames.contains("LP-PPA-app-review-board")) {
            int index = instOriginNames.indexOf("LP-PPA-app-review-board");
            if (originNames.size() >= 3)
                originNames.move(index, 2); // Move to third spot
        }
    }

    if (instOriginNames.contains("Debian")) {
        int index = instOriginNames.indexOf("Debian");
        instOriginNames.move(index, 0); // Move to front of the list
    }

    parentItem = installedItem;
    foreach(const QString & originName, instOriginNames) {
        // We must spread the word of Origin. Hallowed are the Ori! ;P
        QString originLabel = m_backend->originLabel(originName);
        QStandardItem *viewItem = new QStandardItem;
        viewItem->setEditable(false);
        viewItem->setText(originLabel);
        viewItem->setData(AbstractResource::State::Installed, StateFilterRole);
        viewItem->setData(originName, OriginFilterRole);
        viewItem->setData(AppView, ViewTypeRole);

        if (originName == "Ubuntu") {
            viewItem->setText(i18nc("@item:inlistbox", "Provided by Kubuntu"));
            viewItem->setIcon(KIcon("ubuntu-logo"));
        }

        if (originName == "Canonical") {
            viewItem->setText(i18nc("@item:inlistbox The name of the repository provided by Canonical, Ltd. ",
                                    "Canonical Partners"));
            viewItem->setIcon(KIcon("partner"));
        }

        if (originName.startsWith(QLatin1String("LP-PPA"))) {
            if (originName == QLatin1String("LP-PPA-app-review-board")) {
                viewItem->setText(i18nc("@item:inlistbox An independent software source",
                                        "Independent"));
            }
            viewItem->setIcon(KIcon("user-identity"));
        }

        installedItem->appendRow(viewItem);
        m_viewHash[viewItem->index()] = 0;
    }

    parentItem = m_viewModel->invisibleRootItem();

    QStandardItem *historyItem = new QStandardItem;
    historyItem->setEditable(false);
    historyItem->setIcon(KIcon("view-history").pixmap(32,32));
    historyItem->setText(i18nc("@item:inlistbox Item for showing the history view", "History"));
    historyItem->setData(History, ViewTypeRole);
    parentItem->appendRow(historyItem);
    m_viewHash[historyItem->index()] = 0;

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

            view = new ApplicationListView(this, index);
            ApplicationListView *appView = static_cast<ApplicationListView *>(view);
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
    MuonMainWindow::runSourcesEditor(true);
}

void MainWindow::sourcesEditorFinished(int reload)
{
    m_appBackend->reload();
    clearViews();
    populateViews();
    MuonMainWindow::sourcesEditorFinished(reload);
}

void MainWindow::showLauncherMessage()
{
    clearMessageActions();

    QVector<KService::Ptr> apps;
    foreach (Application *app, m_appBackend->launchList()) {
        app->clearPackage();
        app->package(); // Regenerate package
        if (!app->isInstalled()) {
            continue;
        }
        apps << app->executables();
    }

    m_appBackend->clearLaunchList();
    m_launchableApps = apps;

    if (apps.size() == 1) {
        KService::Ptr service = apps.first();
        QString name = service->genericName().isEmpty() ?
                       service->property("Name").toString() :
                       service->property("Name").toString() % QLatin1Literal(" - ") % service->genericName();
        m_launcherMessage->setText(i18nc("@info", "%1 was successfully installed.", name));

        KAction *launchAction = new KAction(KIcon(service->icon()), i18nc("@action", "Start"), this);
        connect(launchAction, SIGNAL(activated()), this, SLOT(launchSingleApp()));

        m_launcherMessage->addAction(launchAction);
        m_launcherMessage->animatedShow();
    } else if (apps.size() > 1) {
        m_launcherMessage->setText(i18nc("@info", "Applications successfully installed."));
        KAction *launchAction = new KAction(i18nc("@action", "Run New Applications..."), this);
        connect(launchAction, SIGNAL(activated()), this, SLOT(showAppLauncher()));
        m_launcherMessage->addAction(launchAction);
        m_launcherMessage->animatedShow();
    }
}

void MainWindow::launchSingleApp()
{
    KToolInvocation::startServiceByDesktopPath(m_launchableApps.first()->desktopEntryPath());
    m_launcherMessage->animatedHide();
    m_launcherMessage->removeAction(m_launcherMessage->actions().first());
}

void MainWindow::showAppLauncher()
{
    if (!m_appLauncher && !m_launchableApps.isEmpty()) {
        m_appLauncher = new ApplicationLauncher(m_appBackend);
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

    QObject *progressView = m_viewHash[m_progressItem->index()];
    if (progressView)
            progressView->deleteLater();

    m_viewHash.remove(m_progressItem->index());
    m_viewModel->removeRow(m_viewModel->indexFromItem(m_progressItem).row());
    m_progressItem = nullptr;
}
