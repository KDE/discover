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
#include "IconColors.h"

// Qt includes
#include <QAction>
#include <QDebug>
#include <QDesktopServices>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickItem>
#include <QScreen>
#include <qqml.h>
#include <QQmlNetworkAccessManagerFactory>
#include <QPointer>
#include <QGuiApplication>

// KDE includes
#include <KAboutApplicationDialog>
#include <KAuthorized>
#include <KBugReport>
#include <KActionCollection>
#include <KDeclarative/KDeclarative>
#include <KLocalizedString>
#include <KMessageBox>
#include <KHelpMenu>
#include <KAboutData>
#include <KHelpMenu>
#include <KShortcutsDialog>
// #include <KSwitchLanguageDialog>

// DiscoverCommon includes
#include <MuonDataSources.h>
#include <resources/ResourcesModel.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>

#include <cmath>

MuonDiscoverMainWindow::MuonDiscoverMainWindow(CompactMode mode)
    : QObject()
    , m_collection(this)
    , m_engine(new QQmlApplicationEngine)
    , m_mode(mode)
{
    ResourcesModel *m = ResourcesModel::global();
    m->integrateActions(actionCollection());

    setObjectName(QStringLiteral("DiscoverMain"));
    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_engine);
    kdeclarative.setupBindings();
    
    qmlRegisterType<PaginateModel>("org.kde.discover.app", 1, 0, "PaginateModel");
    qmlRegisterType<IconColors>("org.kde.discover.app", 1, 0, "IconColors");
    qmlRegisterSingletonType<SystemFonts>("org.kde.discover.app", 1, 0, "SystemFonts", ([](QQmlEngine*, QJSEngine*) -> QObject* { return new SystemFonts; }));
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/DiscoverSystemPalette.qml")), "org.kde.discover.app", 1, 0, "DiscoverSystemPalette");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/Helpers.qml")), "org.kde.discover.app", 1, 0, "Helpers");
    qmlRegisterType<QQuickView>();
    qmlRegisterType<QActionGroup>();
    qmlRegisterType<QAction>();
    qmlRegisterUncreatableType<MuonDiscoverMainWindow>("org.kde.discover.app", 1, 0, "MuonDiscoverMainWindow", QStringLiteral("don't do that"));
    setupActions();
    
    //Here we set up a cache for the screenshots
    m_engine->rootContext()->setContextProperty(QStringLiteral("app"), this);

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this, &MuonDiscoverMainWindow::integrateObject);
    m_engine->load(QUrl(QStringLiteral("qrc:/qml/DiscoverWindow.qml")));
}

MuonDiscoverMainWindow::~MuonDiscoverMainWindow()
{
    delete m_engine;
}

QStringList MuonDiscoverMainWindow::modes() const
{
    QStringList ret;
    QObject* obj = rootObject();
    for(int i = obj->metaObject()->propertyOffset(); i<obj->metaObject()->propertyCount(); i++) {
        QMetaProperty p = obj->metaObject()->property(i);
        QByteArray compName = p.name();
        if(compName.startsWith("top") && compName.endsWith("Comp")) {
            ret += QString::fromLatin1(compName.mid(3, compName.length()-7));
        }
    }
    return ret;
}

void MuonDiscoverMainWindow::openMode(const QByteArray& _mode)
{
    if(!modes().contains(QString::fromLatin1(_mode)))
        qWarning() << "unknown mode" << _mode;
    
    QByteArray mode = _mode;
    if(mode[0]>'Z')
        mode[0] = mode[0]-'a'+'A';
    QObject* obj = rootObject();
    QByteArray propertyName = "top"+mode+"Comp";
    QVariant modeComp = obj->property(propertyName.constData());
    obj->setProperty("currentTopLevel", modeComp);
}

void MuonDiscoverMainWindow::openMimeType(const QString& mime)
{
    emit listMimeInternal(mime);
}

void MuonDiscoverMainWindow::openCategory(const QString& category)
{
    CategoryModel m;
    Category* cat = m.findCategoryByName(category);
    Q_ASSERT(cat);
    emit listCategoryInternal(cat);
}

void MuonDiscoverMainWindow::openApplication(const QString& app)
{
    rootObject()->setProperty("defaultStartup", false);
    m_appToBeOpened = app;
    triggerOpenApplication();
    if(!m_appToBeOpened.isEmpty()) {
        if (ResourcesModel::global()->isFetching()) {
            connect(ResourcesModel::global(), &ResourcesModel::rowsInserted, this, &MuonDiscoverMainWindow::triggerOpenApplication);
        } else {
            triggerOpenApplication();
        }
    }
}

void MuonDiscoverMainWindow::triggerOpenApplication()
{
    AbstractResource* app = ResourcesModel::global()->resourceByPackageName(m_appToBeOpened);
    if(app) {
        emit openApplicationInternal(app);
        m_appToBeOpened.clear();
        disconnect(ResourcesModel::global(), &ResourcesModel::rowsInserted, this, &MuonDiscoverMainWindow::triggerOpenApplication);
    } else {
        qDebug() << "couldn't find" << m_appToBeOpened;
    }
}

QUrl MuonDiscoverMainWindow::featuredSource() const
{
    return MuonDataSources::featuredSource();
}

QUrl MuonDiscoverMainWindow::prioritaryFeaturedSource() const
{
    return QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::DataLocation, QStringLiteral("featured.json")));
}

void MuonDiscoverMainWindow::integrateObject(QObject* object)
{
    KConfigGroup window(KSharedConfig::openConfig(), "Window");
    if (window.hasKey("geometry"))
        rootObject()->setGeometry(window.readEntry("geometry", QRect()));

    object->installEventFilter(this);
}

bool MuonDiscoverMainWindow::eventFilter(QObject * object, QEvent * event)
{
    if (object!=rootObject())
        return false;

    if (event->type() == QEvent::Close) {
        if (ResourcesModel::global()->isBusy()) {
            qWarning() << "not closing because there's still pending tasks";
            Q_EMIT preventedClose();
            return true;
        }

        KConfigGroup window(KSharedConfig::openConfig(), "Window");
        window.writeEntry("geometry", rootObject()->geometry());
    }
    return false;
}

void MuonDiscoverMainWindow::setupActions()
{
    QAction *quitAction = KStandardAction::quit(QCoreApplication::instance(), SLOT(quit()), actionCollection());
    actionCollection()->addAction(QStringLiteral("file_quit"), quitAction);

    QAction* configureSourcesAction = new QAction(QIcon::fromTheme(QStringLiteral("repository")), i18n("Configure Sources"), this);
    connect(configureSourcesAction, &QAction::triggered, this, &MuonDiscoverMainWindow::configureSources);
    actionCollection()->addAction(QStringLiteral("configure_sources"), configureSourcesAction);

    if (KAuthorized::authorizeKAction(QStringLiteral("help_contents"))) {
        auto mHandBookAction = KStandardAction::helpContents(this, SLOT(appHelpActivated()), this);
        actionCollection()->addAction(mHandBookAction->objectName(), mHandBookAction);
    }

    if (KAuthorized::authorizeKAction(QStringLiteral("help_report_bug")) && !KAboutData::applicationData().bugAddress().isEmpty()) {
        auto mReportBugAction = KStandardAction::reportBug(this, SLOT(reportBug()), this);
        actionCollection()->addAction(mReportBugAction->objectName(), mReportBugAction);
    }

    if (KAuthorized::authorizeKAction(QStringLiteral("switch_application_language"))) {
        if (KLocalizedString::availableApplicationTranslations().count() > 1) {
            auto mSwitchApplicationLanguageAction = KStandardAction::create(KStandardAction::SwitchApplicationLanguage, this, SLOT(switchApplicationLanguage()), this);
            actionCollection()->addAction(mSwitchApplicationLanguageAction->objectName(), mSwitchApplicationLanguageAction);
        }
    }

    if (KAuthorized::authorizeKAction(QStringLiteral("help_about_app"))) {
        auto mAboutAppAction = KStandardAction::aboutApp(this, SLOT(aboutApplication()), this);
        actionCollection()->addAction(mAboutAppAction->objectName(), mAboutAppAction);
    }
    auto mKeyBindignsAction = KStandardAction::keyBindings(this, SLOT(configureShortcuts()), this);
    actionCollection()->addAction(mKeyBindignsAction->objectName(), mKeyBindignsAction);
}

QAction * MuonDiscoverMainWindow::action(const QString& name)
{
    return actionCollection()->action(name);
}

QString MuonDiscoverMainWindow::iconName(const QIcon& icon)
{
    return icon.name();
}

void MuonDiscoverMainWindow::configureSources()
{
    openMode("Sources");
}

void MuonDiscoverMainWindow::appHelpActivated()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}

void MuonDiscoverMainWindow::aboutApplication()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KAboutApplicationDialog(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void MuonDiscoverMainWindow::reportBug()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KBugReport(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void MuonDiscoverMainWindow::switchApplicationLanguage()
{
//     auto langDialog = new KSwitchLanguageDialog(nullptr);
//     connect(langDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
//     langDialog->show();
}

void MuonDiscoverMainWindow::configureShortcuts()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, nullptr);
    dlg.setModal(true);
    dlg.addCollection(actionCollection());
    qDebug() << "saving shortcuts..." << dlg.configure(/*bSaveSettings*/);
}

void MuonDiscoverMainWindow::setCompactMode(MuonDiscoverMainWindow::CompactMode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        Q_EMIT compactModeChanged(m_mode);
    }
}

class DiscoverTestExecutor : public QObject
{
public:
    DiscoverTestExecutor(QObject* rootObject, QQmlEngine* engine, const QUrl &url)
        : QObject(engine)
    {
        connect(engine, &QQmlEngine::quit, this, &DiscoverTestExecutor::finish, Qt::QueuedConnection);

        QQmlComponent* component = new QQmlComponent(engine, url, engine);
        m_testObject = component->create(engine->rootContext());

        if (!m_testObject) {
            qWarning() << "error loading test" << url << m_testObject << component->errors();
            Q_ASSERT(false);
        }

        m_testObject->setProperty("appRoot", QVariant::fromValue<QObject*>(rootObject));
        connect(engine, &QQmlEngine::warnings, this, &DiscoverTestExecutor::processWarnings);
    }

    void processWarnings(const QList<QQmlError> &warnings) {
        foreach(const QQmlError &warning, warnings) {
            if (warning.url().path().endsWith(QLatin1String("DiscoverTest.qml"))) {
                qWarning() << "Test failed!" << warnings;
                qGuiApp->exit(1);
            }
        }
        m_warnings << warnings;
    }

    void finish() {
        //The CI doesn't seem to have icons, remove when it's not an issue anymore
        m_warnings.erase(std::remove_if(m_warnings.begin(), m_warnings.end(), [](const QQmlError& err) -> bool {
            return err.description().contains(QLatin1String("QML Image: Failed to get image from provider: image://icon/"));
        }));

        if (m_warnings.isEmpty())
            qDebug() << "cool no warnings!";
        else
            qDebug() << "test finished succesfully despite" << m_warnings;
        qGuiApp->exit(m_warnings.count());
    }

private:
    QObject* m_testObject;
    QList<QQmlError> m_warnings;
};

void MuonDiscoverMainWindow::loadTest(const QUrl& url)
{
    new DiscoverTestExecutor(rootObject(), engine(), url);
}

QWindow* MuonDiscoverMainWindow::rootObject() const
{
    return qobject_cast<QWindow*>(m_engine->rootObjects().at(0));
}
