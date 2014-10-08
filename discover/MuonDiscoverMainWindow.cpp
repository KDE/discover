/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "MuonDiscoverMainWindow.h"
#include "DiscoverAction.h"
#include "PaginateModel.h"
#include "SystemFonts.h"

// Qt includes
#include <QDebug>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickItem>
#include <QTimer>
#include <QGraphicsObject>
#include <QToolButton>
#include <QLayout>
#include <qqml.h>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QQmlNetworkAccessManagerFactory>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QQuickWidget>

// KDE includes
#include <KToolBarSpacerAction>
#include <KActionCollection>
#include <kdeclarative/kdeclarative.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>
#include <KXMLGUIFactory>
#include <KToolBarPopupAction>
#include <KIO/MetaData>
#include <KHelpMenu>
#include <KAboutData>

// Libmuon includes
#include <libmuon/MuonDataSources.h>
#include <resources/ResourcesModel.h>
#include <Category/Category.h>

MuonDiscoverMainWindow::MuonDiscoverMainWindow()
    : MuonMainWindow()
{
    initialize();
    //TODO: reconsider for QtQuick2
//     m_view->setBackgroundRole(QPalette::AlternateBase);
//     qreal bgGrayness = m_view->backgroundBrush().color().blackF();

    m_view = new QQuickWidget(this);
    m_view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QQmlEngine* engine = m_view->engine();
    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(engine);
    //binds things like kconfig and icons
    kdeclarative.setupBindings();
    
    qmlRegisterType<PaginateModel>("org.kde.muon.discover", 1, 0, "PaginateModel");
    qmlRegisterType<DiscoverAction>("org.kde.muon.discover", 1, 0, "DiscoverAction");
    qmlRegisterSingletonType<SystemFonts>("org.kde.muon.discover", 1, 0, "SystemFonts", ([](QQmlEngine*, QJSEngine*) -> QObject* { return new SystemFonts; }));
    qmlRegisterType<KXmlGuiWindow>();
    qmlRegisterType<QActionGroup>();
    qmlRegisterType<QAction>();
    
    m_searchText = new QLineEdit(this);
    m_searchText->setPlaceholderText(i18n("Search..."));
    
    actionCollection()->addAction("edit_find", KStandardAction::find(m_searchText, SLOT(setFocus()), this));
    //Here we set up a cache for the screenshots
    engine->rootContext()->setContextProperty("app", this);
//
//     KConfigGroup window(KSharedConfig::openConfig(), "Window");
//     restoreGeometry(window.readEntry<QByteArray>("geometry", QByteArray()));
//     restoreState(window.readEntry<QByteArray>("windowState", QByteArray()));
    
    m_view->setSource(QUrl("qrc:/qml/Main.qml"));

    if(!m_view->errors().isEmpty()) {
        QString errors;

        for (const QQmlError &error : m_view->errors()) {
            errors.append(error.toString() + QLatin1String("\n"));
        }
        KMessageBox::detailedSorry(this,
                                   i18n("Found some errors while setting up the GUI, the application can't proceed."),
                                   errors, i18n("Initialization error"));
        qDebug() << "errors: " << m_view->errors();
        exit(-1);
    }
    Q_ASSERT(m_view->errors().isEmpty());

    setCentralWidget(m_view);
    setupActions();
}

void MuonDiscoverMainWindow::showEvent(QShowEvent* ev)
{
    QWidget::showEvent(ev);
    m_searchText->setFocus();
}

void MuonDiscoverMainWindow::initialize()
{
    ResourcesModel *m = ResourcesModel::global();
    m->integrateMainWindow(this);
}

MuonDiscoverMainWindow::~MuonDiscoverMainWindow()
{
    delete m_view;
//     KConfigGroup window(KSharedConfig::openConfig(), "Window");
//     window.writeEntry("geometry", saveGeometry());
//     window.writeEntry("windowState", saveState());
//     window.sync();
}

QAction* MuonDiscoverMainWindow::getAction(const QString& name)
{
    return actionCollection()->action(name);
}

QStringList MuonDiscoverMainWindow::modes() const
{
    QStringList ret;
    QObject* obj = m_view->rootObject();
    for(int i = obj->metaObject()->propertyOffset(); i<obj->metaObject()->propertyCount(); i++) {
        QMetaProperty p = obj->metaObject()->property(i);
        QByteArray name = p.name();
        if(name.startsWith("top") && name.endsWith("Comp")) {
            name = name.mid(3);
            name = name.left(name.length()-4);
            name[0] = name[0] - 'A' + 'a';
            ret += name;
        }
    }
    return ret;
}

void MuonDiscoverMainWindow::openMode(const QByteArray& _mode)
{
    if(!modes().contains(_mode))
        qWarning() << "unknown mode" << _mode;
    
    QByteArray mode = _mode;
    if(mode[0]>'Z')
        mode[0] = mode[0]-'a'+'A';
    QObject* obj = m_view->rootObject();
    QByteArray propertyName = "top"+mode+"Comp";
    QVariant modeComp = obj->property(propertyName);
    obj->setProperty("currentTopLevel", modeComp);
}

void MuonDiscoverMainWindow::openMimeType(const QString& mime)
{
    emit listMimeInternal(mime);
}

void MuonDiscoverMainWindow::openCategory(const QString& category)
{
    emit listCategoryInternal(category);
}

void MuonDiscoverMainWindow::openApplication(const QString& app)
{
    m_view->rootObject()->setProperty("defaultStartup", false);
    m_appToBeOpened = app;
    triggerOpenApplication();
    if(!m_appToBeOpened.isEmpty())
        connect(ResourcesModel::global(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(triggerOpenApplication()));
}

void MuonDiscoverMainWindow::triggerOpenApplication()
{
    AbstractResource* app = ResourcesModel::global()->resourceByPackageName(m_appToBeOpened);
    if(app) {
        emit openApplicationInternal(app);
        m_appToBeOpened.clear();
        disconnect(ResourcesModel::global(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(triggerOpenApplication()));
    }
}

QSize MuonDiscoverMainWindow::sizeHint() const
{
    return QSize(800, 900);
}

QUrl MuonDiscoverMainWindow::featuredSource() const
{
    return MuonDataSources::featuredSource();
}

QUrl MuonDiscoverMainWindow::prioritaryFeaturedSource() const
{
    return QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::DataLocation, "featured.json"));
}

void MuonDiscoverMainWindow::setupActions()
{
    setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
    MuonMainWindow::setupActions();

    menuBar()->setVisible(false);
    KHelpMenu* helpMenu = new KHelpMenu(this, KAboutData::applicationData());

    QToolBar* t = toolBar("discoverToolBar");
    QMenu* configMenu = new QMenu(this);
    configMenu->addMenu(qobject_cast<QMenu*>(factory()->container("settings", this)));
    configMenu->addMenu(helpMenu->menu());
    t->setVisible(true);
    
    KToolBarPopupAction* configureButton = new KToolBarPopupAction(QIcon::fromTheme("applications-system"), i18n("Menu"), t);
    configureButton->setToolTip(i18n("Configure and learn about Muon Discover"));
    configureButton->setMenu(configMenu);
    configureButton->setDelayed(false);
    configureButton->setPriority(QAction::LowPriority);
    
    t->addAction(new KToolBarSpacerAction(t));
    t->addWidget(m_searchText);
    t->addAction(configureButton);
}

QObject* MuonDiscoverMainWindow::searchWidget() const
{
    return m_searchText;
}
