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

#include "DiscoverObject.h"
#include "PaginateModel.h"
#include "UnityLauncher.h"
#include "FeaturedModel.h"
#include "CachedNetworkAccessManager.h"
#include "DiscoverDeclarativePlugin.h"

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
#include <KSharedConfig>
#include <KConfigGroup>
// #include <KSwitchLanguageDialog>

// DiscoverCommon includes
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>

#include <functional>
#include <cmath>
#include <unistd.h>
#include <resources/StoredResultsStream.h>
#include <utils.h>

class OurSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
public:
    void classBegin() override {}
    void componentComplete() override {
        if (dynamicSortFilter())
            sort(0);
    }
};

DiscoverObject::DiscoverObject(CompactMode mode)
    : QObject()
    , m_engine(new QQmlApplicationEngine)
    , m_mode(mode)
    , m_networkAccessManagerFactory(new CachedNetworkAccessManagerFactory)
{
    setObjectName(QStringLiteral("DiscoverMain"));
    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_engine);
    kdeclarative.setupBindings();

    qmlRegisterType<UnityLauncher>("org.kde.discover.app", 1, 0, "UnityLauncher");
    qmlRegisterType<PaginateModel>("org.kde.discover.app", 1, 0, "PaginateModel");
    qmlRegisterType<KConcatenateRowsProxyModel>("org.kde.discover.app", 1, 0, "KConcatenateRowsProxyModel");
    qmlRegisterType<FeaturedModel>("org.kde.discover.app", 1, 0, "FeaturedModel");
    qmlRegisterType<OurSortFilterProxyModel>("org.kde.discover.app", 1, 0, "QSortFilterProxyModel");

    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/DiscoverSystemPalette.qml")), "org.kde.discover.app", 1, 0, "DiscoverSystemPalette");
    qmlRegisterType<QQuickView>();
    qmlRegisterType<QActionGroup>();
    qmlRegisterType<QAction>();
    qmlRegisterUncreatableType<DiscoverObject>("org.kde.discover.app", 1, 0, "DiscoverMainWindow", QStringLiteral("don't do that"));
    setupActions();

    auto uri = "org.kde.discover";
    DiscoverDeclarativePlugin* plugin = new DiscoverDeclarativePlugin;
    plugin->setParent(this);
    plugin->initializeEngine(m_engine, uri);
    plugin->registerTypes(uri);

    //Here we set up a cache for the screenshots
    delete m_engine->networkAccessManagerFactory();
    m_engine->setNetworkAccessManagerFactory(m_networkAccessManagerFactory.data());
    m_engine->rootContext()->setContextProperty(QStringLiteral("app"), this);

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this, &DiscoverObject::integrateObject);
    m_engine->load(QUrl(QStringLiteral("qrc:/qml/DiscoverWindow.qml")));
}

DiscoverObject::~DiscoverObject()
{
    delete m_engine;
}

bool DiscoverObject::isRoot()
{
    return ::getuid() == 0;
}

QStringList DiscoverObject::modes() const
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

void DiscoverObject::openMode(const QString& _mode)
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

void DiscoverObject::openMimeType(const QString& mime)
{
    emit listMimeInternal(mime);
}

void DiscoverObject::openCategory(const QString& category)
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

void DiscoverObject::openLocalPackage(const QUrl& localfile)
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

void DiscoverObject::openApplication(const QUrl& url)
{
    Q_ASSERT(!url.isEmpty());
    rootObject()->setProperty("defaultStartup", false);
    auto action = new OneTimeAction(
        [this, url]() {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = url;
            auto stream = new StoredResultsStream({ResourcesModel::global()->search(f)});
            connect(stream, &StoredResultsStream::finished, this, [this, url, stream]() {
                const auto res = stream->resources();
                if (res.count() == 1) {
                    emit openApplicationInternal(res.first());
                } else {
                    rootObject()->setProperty("defaultStartup", true);
                    showPassiveNotification(i18n("Couldn't open %1", url.toDisplayString()));
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

void DiscoverObject::integrateObject(QObject* object)
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
    connect(object, &QObject::destroyed, qGuiApp, &QCoreApplication::quit);
}

bool DiscoverObject::eventFilter(QObject * object, QEvent * event)
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
//     } else if (event->type() == QEvent::ShortcutOverride) {
//         qWarning() << "Action conflict" << event;
    }
    return false;
}

void DiscoverObject::setupActions()
{
    if (KAuthorized::authorizeAction(QStringLiteral("help_contents"))) {
        auto mHandBookAction = KStandardAction::helpContents(this, &DiscoverObject::appHelpActivated, this);
        m_collection[mHandBookAction->objectName()] = mHandBookAction;
    }

    if (KAuthorized::authorizeAction(QStringLiteral("help_report_bug")) && !KAboutData::applicationData().bugAddress().isEmpty()) {
        auto mReportBugAction = KStandardAction::reportBug(this, &DiscoverObject::reportBug, this);
        m_collection[mReportBugAction->objectName()] = mReportBugAction;
    }

    if (KAuthorized::authorizeAction(QStringLiteral("help_about_app"))) {
        auto mAboutAppAction = KStandardAction::aboutApp(this, &DiscoverObject::aboutApplication, this);
        m_collection[mAboutAppAction->objectName()] = mAboutAppAction;
    }
}

QAction * DiscoverObject::action(const QString& name) const
{
    return m_collection.value(name);
}

QString DiscoverObject::iconName(const QIcon& icon)
{
    return icon.name();
}

void DiscoverObject::appHelpActivated()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}

void DiscoverObject::aboutApplication()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KAboutApplicationDialog(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void DiscoverObject::reportBug()
{
    static QPointer<QDialog> dialog;
    if (!dialog) {
        dialog = new KBugReport(KAboutData::applicationData(), nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->show();
}

void DiscoverObject::switchApplicationLanguage()
{
//     auto langDialog = new KSwitchLanguageDialog(nullptr);
//     connect(langDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
//     langDialog->show();
}

void DiscoverObject::setCompactMode(DiscoverObject::CompactMode mode)
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

void DiscoverObject::loadTest(const QUrl& url)
{
    new DiscoverTestExecutor(rootObject(), engine(), url);
}

QWindow* DiscoverObject::rootObject() const
{
    return qobject_cast<QWindow*>(m_engine->rootObjects().at(0));
}

void DiscoverObject::showPassiveNotification(const QString& msg)
{
    QTimer::singleShot(100, this, [this, msg](){
        QMetaObject::invokeMethod(rootObject(), "showPassiveNotification", Qt::QueuedConnection, Q_ARG(QVariant, msg), Q_ARG(QVariant, {}), Q_ARG(QVariant, {}), Q_ARG(QVariant, {}));
    });
}

#include "DiscoverObject.moc"
