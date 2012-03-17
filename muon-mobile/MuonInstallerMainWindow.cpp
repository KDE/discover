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

#include "MuonInstallerMainWindow.h"

// Qt includes
#include <QDebug>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeView>
#include <QDesktopServices>
#include <QTimer>
#include <QGraphicsObject>
#include <qdeclarative.h>
// #if !defined(QT_NO_OPENGL)
//     #include <QGLWidget>
// #endif

// KDE includes
#include <KActionCollection>
#include <KAction>
#include <kdeclarative.h>

// QApt includes
#include <LibQApt/Backend>

// Libmuon includes
#include <Application.h>
#include <ApplicationBackend.h>
#include <Category/CategoryModel.h>
#include <Category/Category.h>
#include <Transaction/TransactionListener.h>
#include <ReviewsBackend/ReviewsBackend.h>
#include <ReviewsBackend/Rating.h>
#include <ApplicationModel/LaunchListModel.h>
#include <ApplicationModel/TransactionsModel.h>
#include <ApplicationModel/ApplicationModel.h>

// Own includes
#include "ApplicationProxyModelHelper.h"
#include "BackendsSingleton.h"
#include "ReviewsModel.h"
#include "ApplicationUpdates.h"

QML_DECLARE_TYPE(ApplicationBackend)

MuonInstallerMainWindow::MuonInstallerMainWindow()
    : MuonMainWindow()
{
    m_view = new QDeclarativeView(this);
    
    KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_view->engine());
    kdeclarative.initialize();
    //binds things like kconfig and icons
    kdeclarative.setupBindings();
    
    qmlRegisterType<CategoryModel>("org.kde.muon", 1, 0, "CategoryModel");
    qmlRegisterType<ApplicationProxyModelHelper>("org.kde.muon", 1, 0, "ApplicationProxyModel");
    qmlRegisterType<TransactionListener>("org.kde.muon", 1, 0, "TransactionListener");
    qmlRegisterType<ReviewsModel>("org.kde.muon", 1, 0, "ReviewsModel");
    qmlRegisterType<ApplicationUpdates>("org.kde.muon", 1, 0, "ApplicationUpdates");
    qmlRegisterType<LaunchListModel>("org.kde.muon", 1, 0, "LaunchListModel");
    qmlRegisterType<TransactionsModel>("org.kde.muon", 1, 0, "TransactionsModel");
    qmlRegisterType<ReviewsBackend>();
    qmlRegisterType<Rating>();
    qmlRegisterType<Application>();
    qmlRegisterType<Category>();
    qmlRegisterType<ApplicationBackend>();
    qmlRegisterType<ApplicationModel>();
    
    connect(this, SIGNAL(backendReady(QApt::Backend*)), SLOT(setBackend(QApt::Backend*)));
    
    m_view->engine()->rootContext()->setContextProperty("app", this);
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
// #if !defined(QT_NO_OPENGL)
//     m_view->setViewport(new QGLWidget);
// #endif
    QTimer::singleShot(10, this, SLOT(initObject()));
    setupActions();
    m_view->setSource(QUrl("qrc:/qml/Main.qml"));
    if(!m_view->errors().isEmpty())
        qDebug() << "errors: " << m_view->errors();
    Q_ASSERT(m_view->errors().isEmpty());
    m_view->rootObject()->setProperty("state", "loading");
    
    setCentralWidget(m_view);
}

void MuonInstallerMainWindow::setBackend(QApt::Backend* b)
{
    BackendsSingleton::self()->initialize(b, this);
    appBackend(); //here we force the retrieval of the appbackend to get ratings
    emit appBackendChanged();
    m_view->rootObject()->setProperty("state", "loaded");
}

ApplicationBackend* MuonInstallerMainWindow::appBackend() const
{
    return BackendsSingleton::self()->applicationBackend();
}

bool MuonInstallerMainWindow::openUrl(const QUrl& url)
{
    return QDesktopServices::openUrl(url);
}

void MuonInstallerMainWindow::errorOccurred(QApt::ErrorCode code, const QVariantMap& args)
{
    MuonMainWindow::errorOccurred(code, args);
}

QAction* MuonInstallerMainWindow::getAction(const QString& name)
{
    return actionCollection()->action(name);
}
