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
#include "NativeScrollBar.h"

// Qt includes
#include <QDebug>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeView>
#include <QDesktopServices>
#include <QTimer>
#include <QGraphicsObject>
#include <QToolButton>
#include <QLayout>
#include <qdeclarative.h>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QDeclarativeNetworkAccessManagerFactory>

// #if !defined(QT_NO_OPENGL)
//     #include <QGLWidget>
// #endif

// KDE includes
#include <KActionCollection>
#include <KAction>
#include <kdeclarative.h>
#include <Plasma/Theme>
#include <KStandardDirs>
#include <KMessageBox>
#include <KToolBar>
#include <KLineEdit>
#include <KMenu>
#include <KMenuBar>
#include <KXMLGUIFactory>
#include <KToolBarPopupAction>

// Libmuon includes
#include <libmuon/MuonDataSources.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesUpdatesModel.h>
#include <Category/CategoryModel.h>
#include <Category/Category.h>
#include <Transaction/TransactionListener.h>
#include <Transaction/Transaction.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <MuonBackendsFactory.h>

Q_DECLARE_METATYPE(ResourcesModel*);

MuonDiscoverMainWindow::MuonDiscoverMainWindow()
    : MuonMainWindow()
{
    initialize();
    m_view = new QDeclarativeView(this);
    m_view->setBackgroundRole(QPalette::AlternateBase);
    
    Plasma::Theme::defaultTheme()->setUseGlobalSettings(false); //don't change every plasma theme!
    Plasma::Theme::defaultTheme()->setThemeName("appdashboard");
    
    KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_view->engine());
    kdeclarative.initialize();
    //binds things like kconfig and icons
    kdeclarative.setupBindings();
    
    qmlRegisterType<DiscoverAction>("org.kde.muon.discover", 1, 0, "DiscoverAction");
    qmlRegisterType<NativeScrollBar>("org.kde.muon.discover", 1, 0, "NativeScrollBar");
    qmlRegisterType<KXmlGuiWindow>();
    
    m_searchText = new KLineEdit;
    m_searchText->setPlaceholderText(i18n("Search..."));
    
    //Here we set up a cache for the screenshots
    m_view->engine()->rootContext()->setContextProperty("resourcesModel",
                                                        qVariantFromValue<ResourcesModel*>(ResourcesModel::global()));
    m_view->engine()->rootContext()->setContextProperty("app", this);
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    
    m_view->setSource(QUrl("qrc:/qml/Main.qml"));
    if(!m_view->errors().isEmpty()) {
        QString errors;

        for (const QDeclarativeError &error : m_view->errors()) {
            errors.append(error.toString() + QLatin1String("\n"));
        }
        KMessageBox::detailedSorry(this,
            i18n("Found some errors while setting up the GUI, the application can't proceed."),
            errors, i18n("Initialization error"));
        qDebug() << "errors: " << m_view->errors();
        exit(-1);
    }
    Q_ASSERT(m_view->errors().isEmpty());

    KConfigGroup window(componentData().config(), "Window");
    restoreGeometry(window.readEntry<QByteArray>("geometry", QByteArray()));
    restoreState(window.readEntry<QByteArray>("windowState", QByteArray()));
    
    setCentralWidget(m_view);
    setupActions();
}

void MuonDiscoverMainWindow::initialize()
{
    MuonBackendsFactory factory;
    QList<AbstractResourcesBackend*> backends = factory.allBackends();
    
    ResourcesModel* m = ResourcesModel::global();
    foreach(AbstractResourcesBackend* b, backends) {
        m->addResourcesBackend(b);
        b->integrateMainWindow(this);
    }
}

MuonDiscoverMainWindow::~MuonDiscoverMainWindow()
{
    KConfigGroup window(componentData().config(), "Window");
    window.writeEntry("geometry", saveGeometry());
    window.writeEntry("windowState", saveState());
    window.sync();
}

QAction* MuonDiscoverMainWindow::getAction(const QString& name)
{
    return actionCollection()->action(name);
}

QStringList MuonDiscoverMainWindow::modes() const
{
    QStringList ret;
    QGraphicsObject* obj = m_view->rootObject();
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
        kWarning() << "unknown mode" << _mode;
    
    QByteArray mode = _mode;
    if(mode[0]>'Z')
        mode[0] = mode[0]-'a'+'A';
    QGraphicsObject* obj = m_view->rootObject();
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
    return QUrl::fromLocalFile(KGlobal::dirs()->findResource("appdata", "featured.json"));
}

void MuonDiscoverMainWindow::setupActions()
{
    setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar));
    MuonMainWindow::setupActions();

    menuBar()->setVisible(false);

    QToolBar* t = toolBar();
    QMenu* configMenu = new QMenu(this);
    configMenu->addMenu(qobject_cast<QMenu*>(factory()->container("settings", this)));
    configMenu->addMenu(helpMenu());
    
    KToolBarPopupAction* configureButton = new KToolBarPopupAction(KIcon("configure"), i18n("Configure"), t);
    configureButton->setMenu(configMenu);
    configureButton->setDelayed(false);
    
    QWidget* wideWidget = new QWidget(t);
    t->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    t->addWidget(wideWidget);
    t->addWidget(m_searchText);
    t->addAction(configureButton);
}

QObject* MuonDiscoverMainWindow::searchWidget() const
{
    return m_searchText;
}
