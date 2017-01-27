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

#include "DiscoverMainWindow.h"
#include "PaginateModel.h"
#include "SystemFonts.h"
#include "IconColors.h"
#include "UnityLauncher.h"
#include "FeaturedModel.h"

// Qt includes
#include <QAction>
#include <QDebug>
#include <QDesktopServices>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickItem>
#include <qqml.h>
#include <QPointer>
#include <QGuiApplication>
#include <QSortFilterProxyModel>
#include <QTimer>

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
#include <KConcatenateRowsProxyModel>
// #include <KSwitchLanguageDialog>

// DiscoverCommon includes
#include <MuonDataSources.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>

#include <functional>
#include <cmath>
#include <unistd.h>
#include <resources/StoredResultsStream.h>

class OneTimeAction : public QObject
{
public:
    OneTimeAction(std::function<void()> func, QObject* parent) : QObject(parent), m_function(func) {}

    void trigger() {
        m_function();
        deleteLater();
    }

private:
    std::function<void()> m_function;
};

DiscoverMainWindow::DiscoverMainWindow(CompactMode mode)
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
    
    qmlRegisterType<UnityLauncher>("org.kde.discover.app", 1, 0, "UnityLauncher");
    qmlRegisterType<PaginateModel>("org.kde.discover.app", 1, 0, "PaginateModel");
    qmlRegisterType<IconColors>("org.kde.discover.app", 1, 0, "IconColors");
    qmlRegisterType<KConcatenateRowsProxyModel>("org.kde.discover.app", 1, 0, "KConcatenateRowsProxyModel");
    qmlRegisterType<FeaturedModel>("org.kde.discover.app", 1, 0, "FeaturedModel");
    qmlRegisterType<QSortFilterProxyModel>("org.kde.discover.app", 1, 0, "QSortFilterProxyModel");

    qmlRegisterSingletonType<SystemFonts>("org.kde.discover.app", 1, 0, "SystemFonts", ([](QQmlEngine*, QJSEngine*) -> QObject* { return new SystemFonts; }));
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/DiscoverSystemPalette.qml")), "org.kde.discover.app", 1, 0, "DiscoverSystemPalette");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/Helpers.qml")), "org.kde.discover.app", 1, 0, "Helpers");
    qmlRegisterType<QQuickView>();
    qmlRegisterType<QActionGroup>();
    qmlRegisterType<QAction>();
    qmlRegisterUncreatableType<DiscoverMainWindow>("org.kde.discover.app", 1, 0, "DiscoverMainWindow", QStringLiteral("don't do that"));
    setupActions();
    
    //Here we set up a cache for the screenshots
    m_engine->rootContext()->setContextProperty(QStringLiteral("app"), this);

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this, &DiscoverMainWindow::integrateObject);
    m_engine->load(QUrl(QStringLiteral("qrc:/qml/DiscoverWindow.qml")));
}

DiscoverMainWindow::~DiscoverMainWindow()
{
    delete m_engine;
}

bool DiscoverMainWindow::isRoot()
{
    return ::getuid() == 0;
}

QStringList DiscoverMainWindow::modes() const
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

void DiscoverMainWindow::openMode(const QString& _mode)
{
    if(!modes().contains(_mode))
        qWarning() << "unknown mode" << _mode;
    
    QString mode = _mode;
    mode[0] = mode[0].toUpper();

    QObject* obj = rootObject();
    const QByteArray propertyName = "top"+mode.toLatin1()+"Comp";
    const QVariant modeComp = obj->property(propertyName.constData());
    if (!modeComp.isValid())
        qWarning() << "couldn't open mode" << _mode;
    else
        obj->setProperty("currentTopLevel", modeComp);
}

void DiscoverMainWindow::openMimeType(const QString& mime)
{
    emit listMimeInternal(mime);
}

void DiscoverMainWindow::openCategory(const QString& category)
{
    rootObject()->setProperty("defaultStartup", false);
    auto action = new OneTimeAction(
        [this, category]() {
            Category* cat = CategoryModel::global()->findCategoryByName(category);
            if (cat) {
                emit listCategoryInternal(cat);
            } else {
                showPassiveNotification(i18n("Could not find category '%1'", category));
                rootObject()->setProperty("defaultStartup", false);
            }
        }
        , this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverMainWindow::openLocalPackage(const QUrl& localfile)
{
    if (!QFile::exists(localfile.toLocalFile())) {
//         showPassiveNotification(i18n("Trying to open unexisting file '%1'", localfile.toString()));
        qWarning() << "Trying to open unexisting file" << localfile;
        return;
    }
    rootObject()->setProperty("defaultStartup", false);
    auto action = new OneTimeAction(
        [this, localfile]() {
            auto res = ResourcesModel::global()->resourceForFile(localfile);
            qDebug() << "all initialized..." << res;
            if (res) {
                emit openApplicationInternal(res);
            } else {
                rootObject()->setProperty("defaultStartup", true);
                showPassiveNotification(i18n("Couldn't open %1", localfile.toDisplayString()));
            }
        }
        , this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverMainWindow::openApplication(const QUrl& app)
{
    Q_ASSERT(!app.isEmpty());
    rootObject()->setProperty("defaultStartup", false);
    auto action = new OneTimeAction(
        [this, app]() {
            StoredResultsStream* stream = new StoredResultsStream({ResourcesModel::global()->findResourceByPackageName(app)});
            connect(stream, &StoredResultsStream::finishedResources, stream, [this, stream, app](const QVector<AbstractResource*> &resources) {
                if (!resources.isEmpty()) {
                    if (resources.size() > 1)
                        qWarning() << "many resources found for" << app;
                    emit openApplicationInternal(resources.first());
                } else {
                    rootObject()->setProperty("defaultStartup", true);
                    showPassiveNotification(i18n("Couldn't open %1", app.toDisplayString()));
                }
            });
        }
        , this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

QUrl DiscoverMainWindow::featuredSource() const
{
    return MuonDataSources::featuredSource();
}

QUrl DiscoverMainWindow::prioritaryFeaturedSource() const
{
    return QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("plasmadiscover/featured.json")));
}

void DiscoverMainWindow::integrateObject(QObject* object)
{
    if (!object) {
        qWarning() << "Errors when loading the GUI";
        QTimer::singleShot(0, QCoreApplication::instance(), [](){
            QCoreApplication::instance()->exit(1);
        });
        return;
    }

    Q_ASSERT(object == rootObject());

    KConfigGroup window(KSharedConfig::openConfig(), "Window");
    if (window.hasKey("geometry"))
        rootObject()->setGeometry(window.readEntry("geometry", QRect()));
    if (window.hasKey("visibility")) {
        QWindow::Visibility visibility(QWindow::Visibility(window.readEntry<int>("visibility", QWindow::Windowed)));
        rootObject()->setVisibility(qMax(visibility, QQuickView::AutomaticVisibility));
    }

    object->installEventFilter(this);
}

bool DiscoverMainWindow::eventFilter(QObject * object, QEvent * event)
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
        window.writeEntry<int>("visibility", rootObject()->visibility());
    }
    return false;
}

void DiscoverMainWindow::setupActions()
{
    QAction *quitAction = KStandardAction::quit(QCoreApplication::instance(), &QCoreApplication::quit, actionCollection());
    actionCollection()->addAction(QStringLiteral("file_quit"), quitAction);

    if (KAuthorized::authorizeAction(QStringLiteral("help_contents"))) {
        auto mHandBookAction = KStandardAction::helpContents(this, &DiscoverMainWindow::appHelpActivated, this);
        actionCollection()->addAction(mHandBookAction->objectName(), mHandBookAction);
    }

    if (KAuthorized::authorizeAction(QStringLiteral("help_report_bug")) && !KAboutData::applicationData().bugAddress().isEmpty()) {
        auto mReportBugAction = KStandardAction::reportBug(this, &DiscoverMainWindow::reportBug, this);
        actionCollection()->addAction(mReportBugAction->objectName(), mReportBugAction);
    }

    if (KAuthorized::authorizeAction(QStringLiteral("switch_application_language"))) {
//         if (KLocalizedString::availableApplicationTranslations().count() > 1) {
            auto mSwitchApplicationLanguageAction = KStandardAction::create(KStandardAction::SwitchApplicationLanguage, this, &DiscoverMainWindow::switchApplicationLanguage, this);
            actionCollection()->addAction(mSwitchApplicationLanguageAction->objectName(), mSwitchApplicationLanguageAction);
//         }
    }

    if (KAuthorized::authorizeAction(QStringLiteral("help_about_app"))) {
        auto mAboutAppAction = KStandardAction::aboutApp(this, &DiscoverMainWindow::aboutApplication, this);
        actionCollection()->addAction(mAboutAppAction->objectName(), mAboutAppAction);
    }
    auto mKeyBindignsAction = KStandardAction::keyBindings(this, &DiscoverMainWindow::configureShortcuts, this);
    actionCollection()->addAction(mKeyBindignsAction->objectName(), mKeyBindignsAction);
}

QAction * DiscoverMainWindow::action(const QString& name)
{
    return actionCollection()->action(name);
}

QString DiscoverMainWindow::iconName(const QIcon& icon)
{
    return icon.name();
}

void DiscoverMainWindow::appHelpActivated()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}

void DiscoverMainWindow::aboutApplication()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KAboutApplicationDialog(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void DiscoverMainWindow::reportBug()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KBugReport(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void DiscoverMainWindow::switchApplicationLanguage()
{
//     auto langDialog = new KSwitchLanguageDialog(nullptr);
//     connect(langDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
//     langDialog->show();
}

void DiscoverMainWindow::configureShortcuts()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, nullptr);
    dlg.setModal(true);
    dlg.addCollection(actionCollection());
    qDebug() << "saving shortcuts..." << dlg.configure(/*bSaveSettings*/);
}

void DiscoverMainWindow::setCompactMode(DiscoverMainWindow::CompactMode mode)
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

void DiscoverMainWindow::loadTest(const QUrl& url)
{
    new DiscoverTestExecutor(rootObject(), engine(), url);
}

QWindow* DiscoverMainWindow::rootObject() const
{
    return qobject_cast<QWindow*>(m_engine->rootObjects().at(0));
}

void DiscoverMainWindow::showPassiveNotification(const QString& msg)
{
    QTimer::singleShot(100, this, [this, msg](){
        QMetaObject::invokeMethod(rootObject(), "showPassiveNotification", Qt::QueuedConnection, Q_ARG(QVariant, msg), Q_ARG(QVariant, {}), Q_ARG(QVariant, {}), Q_ARG(QVariant, {}));
    });
}
