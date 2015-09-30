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
#include <QQuickWidget>
#include <QScreen>

// KDE includes
#include <KActionCollection>
#include <kdeclarative/kdeclarative.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>
#include <KXMLGUIFactory>
#include <KIO/MetaData>
#include <KHelpMenu>
#include <KAboutData>

// Libmuon includes
#include <libmuon/MuonDataSources.h>
#include <resources/ResourcesModel.h>
#include <resources/UIHelper.h>
#include <Category/Category.h>

#include <cmath>

MuonDiscoverMainWindow::MuonDiscoverMainWindow()
    : KXmlGuiWindow()
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
    qmlRegisterSingletonType<SystemFonts>("org.kde.muon.discover", 1, 0, "SystemFonts", ([](QQmlEngine*, QJSEngine*) -> QObject* { return new SystemFonts; }));
    qmlRegisterType<KXmlGuiWindow>();
    qmlRegisterType<QActionGroup>();
    qmlRegisterType<QAction>();
    
    //Here we set up a cache for the screenshots
    engine->rootContext()->setContextProperty("app", this);
//
//     KConfigGroup window(KSharedConfig::openConfig(), "Window");
//     restoreGeometry(window.readEntry<QByteArray>("geometry", QByteArray()));
//     restoreState(window.readEntry<QByteArray>("windowState", QByteArray()));
    
    m_view->setSource(QUrl("qrc:/qml/Main.qml"));

    if(!m_view->errors().isEmpty()) {
        QString errors;

        Q_FOREACH (const QQmlError &error, m_view->errors()) {
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

bool MuonDiscoverMainWindow::isCompact() const
{
    if (!isVisible())
        return true;

    const qreal pixelDensity = windowHandle()->screen()->physicalDotsPerInch() / 25.4;
    return (width()/pixelDensity)<70; //we'll use compact if the width of the window is less than 7cm
}

qreal MuonDiscoverMainWindow::actualWidth() const
{
    return isCompact() ? width() : width()-std::pow(width()/70, 2);
}

void MuonDiscoverMainWindow::resizeEvent(QResizeEvent * event)
{
    KXmlGuiWindow::resizeEvent(event);
    Q_EMIT compactChanged(isCompact());
    Q_EMIT actualWidthChanged(actualWidth());
}

void MuonDiscoverMainWindow::showEvent(QShowEvent * event)
{
    KXmlGuiWindow::showEvent(event);
    Q_EMIT compactChanged(isCompact());
    Q_EMIT actualWidthChanged(actualWidth());
}

void MuonDiscoverMainWindow::setupActions()
{
    setupGUI(StandardWindowOption(KXmlGuiWindow::Default & ~KXmlGuiWindow::StatusBar & ~KXmlGuiWindow::ToolBar));

    QAction *quitAction = KStandardAction::quit(QCoreApplication::instance(), SLOT(quit()), actionCollection());
    actionCollection()->addAction("file_quit", quitAction);

    menuBar()->setVisible(false);
    toolBar("discoverToolBar")->setVisible(false);

    m_moreMenu = new QMenu(this);
    m_advancedMenu = new QMenu(i18n("Advanced..."), m_moreMenu);
    configureMenu();

    connect(ResourcesModel::global(), &ResourcesModel::allInitialized, this, &MuonDiscoverMainWindow::configureMenu);
}

void MuonDiscoverMainWindow::configureMenu()
{
    m_advancedMenu->clear();
    m_moreMenu->clear();
    UIHelper::setupMessageActions(m_moreMenu, m_advancedMenu, ResourcesModel::global()->messageActions());

    if (!m_moreMenu->isEmpty())
        m_moreMenu->addSeparator();

    m_moreMenu->addAction(actionCollection()->action("options_configure_keybinding"));
    m_moreMenu->addSeparator();
    m_moreMenu->addMenu(m_advancedMenu);
    m_moreMenu->addSeparator();
    m_moreMenu->addAction(actionCollection()->action("help_about_app"));
    m_moreMenu->addAction(actionCollection()->action("help_about_kde"));
    m_moreMenu->addAction(actionCollection()->action("help_report_bug"));
}

bool MuonDiscoverMainWindow::queryClose()
{
    return !ResourcesModel::global()->isBusy();
}

void MuonDiscoverMainWindow::showMenu(int x, int y)
{
    QPoint p = m_view->mapToGlobal(QPoint(x, y));
    m_moreMenu->exec(p);
}
