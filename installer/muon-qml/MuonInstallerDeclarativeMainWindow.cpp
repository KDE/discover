/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "MuonInstallerDeclarativeMainWindow.h"
#include <kdeclarative.h>
#include <qdeclarative.h>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <CategoryModel.h>
#include <CategoryView/Category.h>
#include <ApplicationBackend.h>
#include <LibQApt/Backend>
#include <QDebug>
#include <QTimer>
#include <qaction.h>
#include <KActionCollection>
#include "ApplicationProxyModelHelper.h"
#include "BackendsSingleton.h"

Q_DECLARE_METATYPE(ApplicationBackend*)

MuonInstallerMainWindow::MuonInstallerMainWindow()
    : MuonMainWindow()
{
    QDeclarativeView* view = new QDeclarativeView(this);
    
    KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(view->engine());
    kdeclarative.initialize();
    //binds things like kconfig and icons
    kdeclarative.setupBindings();
    
    qmlRegisterType<CategoryModel>("org.kde.muon", 1, 0, "CategoryModel");
    qmlRegisterType<ApplicationProxyModelHelper>("org.kde.muon", 1, 0, "ApplicationProxyModel");
    qmlRegisterInterface<Category>("Category");
    qmlRegisterInterface<ApplicationBackend>("ApplicationBackend");
    
    connect(actionCollection(), SIGNAL(inserted(QAction*)), SIGNAL(actionsChanged()));
    connect(actionCollection(), SIGNAL(removed(QAction*)), SIGNAL(actionsChanged()));
    connect(this, SIGNAL(backendReady(QApt::Backend*)), SLOT(setBackend(QApt::Backend*)));
    
    view->engine()->rootContext()->setContextProperty("app", this);
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/qml/Main.qml"));
    
    QTimer::singleShot(10, this, SLOT(initObject()));
    setupActions();
    setCentralWidget(view);
}

QVariantList MuonInstallerMainWindow::actions() const
{
    QList<QAction*> acts = actionCollection()->actions();
    QVariantList ret;
    foreach(QAction* a, acts) {
        ret += qVariantFromValue<QObject*>(a);
    }
    return ret;
}

void MuonInstallerMainWindow::setBackend(QApt::Backend* b)
{
    BackendsSingleton::self()->setBackend(b);
}
