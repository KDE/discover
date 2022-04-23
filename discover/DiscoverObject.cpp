/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverObject.h"
#include "CachedNetworkAccessManager.h"
#include "DiscoverBackendsFactory.h"
#include "DiscoverDeclarativePlugin.h"
#include "FeaturedModel.h"
#include "PaginateModel.h"
#include "UnityLauncher.h"

// Qt includes
#include "discover_debug.h"
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSessionManager>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QtQuick/QQuickItem>
#include <qqml.h>

// KDE includes
#include <KAboutData>
#include <KAuthorized>
#include <KConfigGroup>
#include <KCrash>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KSharedConfig>
#include <kcoreaddons_version.h>
// #include <KSwitchLanguageDialog>

// DiscoverCommon includes
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>

#include <QMimeDatabase>
#include <cmath>
#include <functional>
#include <resources/StoredResultsStream.h>
#include <unistd.h>
#include <utils.h>

#ifdef WITH_FEEDBACK
#include "plasmauserfeedback.h"
#endif
#include "discoversettings.h"

class CachedNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
    virtual QNetworkAccessManager *create(QObject *parent) override
    {
        return new CachedNetworkAccessManager(QStringLiteral("images"), parent);
    }
};

class OurSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    void classBegin() override
    {
    }
    void componentComplete() override
    {
        if (dynamicSortFilter())
            sort(0);
    }
};

DiscoverObject::DiscoverObject(CompactMode mode, const QVariantMap &initialProperties)
    : QObject()
    , m_engine(new QQmlApplicationEngine)
    , m_mode(mode)
    , m_networkAccessManagerFactory(new CachedNetworkAccessManagerFactory)
{
    setObjectName(QStringLiteral("DiscoverMain"));
    m_engine->rootContext()->setContextObject(new KLocalizedContext(m_engine));
    auto factory = m_engine->networkAccessManagerFactory();
    m_engine->setNetworkAccessManagerFactory(nullptr);
    delete factory;
    m_engine->setNetworkAccessManagerFactory(m_networkAccessManagerFactory.data());

    qmlRegisterType<UnityLauncher>("org.kde.discover.app", 1, 0, "UnityLauncher");
    qmlRegisterType<PaginateModel>("org.kde.discover.app", 1, 0, "PaginateModel");
    qmlRegisterType<FeaturedModel>("org.kde.discover.app", 1, 0, "FeaturedModel");
    qmlRegisterType<OurSortFilterProxyModel>("org.kde.discover.app", 1, 0, "QSortFilterProxyModel");
#ifdef WITH_FEEDBACK
    qmlRegisterSingletonType<PlasmaUserFeedback>("org.kde.discover.app", 1, 0, "UserFeedbackSettings", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new PlasmaUserFeedback(KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback"), KConfig::NoGlobals));
    });
#endif
    qmlRegisterSingletonType<DiscoverSettings>("org.kde.discover.app", 1, 0, "DiscoverSettings", [](QQmlEngine *, QJSEngine *) -> QObject * {
        auto r = new DiscoverSettings;
        connect(r, &DiscoverSettings::installedPageSortingChanged, r, &DiscoverSettings::save);
        connect(r, &DiscoverSettings::appsListPageSortingChanged, r, &DiscoverSettings::save);
        return r;
    });
    qmlRegisterAnonymousType<QQuickView>("org.kde.discover.app", 1);

    qmlRegisterAnonymousType<KAboutData>("org.kde.discover.app", 1);
    qmlRegisterAnonymousType<KAboutLicense>("org.kde.discover.app", 1);
    qmlRegisterAnonymousType<KAboutPerson>("org.kde.discover.app", 1);

    qmlRegisterUncreatableType<DiscoverObject>("org.kde.discover.app", 1, 0, "DiscoverMainWindow", QStringLiteral("don't do that"));

    auto uri = "org.kde.discover";
    DiscoverDeclarativePlugin *plugin = new DiscoverDeclarativePlugin;
    plugin->setParent(this);
    plugin->initializeEngine(m_engine, uri);
    plugin->registerTypes(uri);

    m_engine->setInitialProperties(initialProperties);
    m_engine->rootContext()->setContextProperty(QStringLiteral("app"), this);
    m_engine->rootContext()->setContextProperty(QStringLiteral("discoverAboutData"), QVariant::fromValue(KAboutData::applicationData()));

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this, &DiscoverObject::integrateObject);
    m_engine->load(QUrl(QStringLiteral("qrc:/qml/DiscoverWindow.qml")));

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        const auto objs = m_engine->rootObjects();
        for (auto o : objs)
            delete o;
    });
    auto action = new OneTimeAction(
        [this]() {
            bool found = DiscoverBackendsFactory::hasRequestedBackends();
            const auto backends = ResourcesModel::global()->backends();
            for (auto b : backends)
                found |= b->hasApplications();

            if (!found)
                Q_EMIT openErrorPage(
                    i18n("Discover currently cannot be used to install any apps "
                         "because none of its app backends are available. Please "
                         "report this issue to the packagers of your distribution."));
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::allInitialized, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
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
    QObject *obj = rootObject();
    for (int i = obj->metaObject()->propertyOffset(); i < obj->metaObject()->propertyCount(); i++) {
        QMetaProperty p = obj->metaObject()->property(i);
        QByteArray compName = p.name();
        if (compName.startsWith("top") && compName.endsWith("Comp")) {
            ret += QString::fromLatin1(compName.mid(3, compName.length() - 7));
        }
    }
    return ret;
}

void DiscoverObject::openMode(const QString &_mode)
{
    QObject *obj = rootObject();
    if (!obj) {
        qCWarning(DISCOVER_LOG) << "could not get the main object";
        return;
    }

    if (!modes().contains(_mode, Qt::CaseInsensitive))
        qCWarning(DISCOVER_LOG) << "unknown mode" << _mode << modes();

    QString mode = _mode;
    mode[0] = mode[0].toUpper();

    const QByteArray propertyName = "top" + mode.toLatin1() + "Comp";
    const QVariant modeComp = obj->property(propertyName.constData());
    if (!modeComp.isValid())
        qCWarning(DISCOVER_LOG) << "couldn't open mode" << _mode;
    else
        obj->setProperty("currentTopLevel", modeComp);
}

void DiscoverObject::openMimeType(const QString &mime)
{
    Q_EMIT listMimeInternal(mime);
}

void DiscoverObject::showLoadingPage()
{
    QObject *obj = rootObject();
    if (!obj) {
        qCWarning(DISCOVER_LOG) << "could not get the main object";
        return;
    }

    obj->setProperty("currentTopLevel", QStringLiteral("qrc:/qml/LoadingPage.qml"));
}

void DiscoverObject::openCategory(const QString &category)
{
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, category]() {
            Category *cat = CategoryModel::global()->findCategoryByName(category);
            if (cat) {
                Q_EMIT listCategoryInternal(cat);
            } else {
                openMode(QStringLiteral("Browsing"));
                showPassiveNotification(i18n("Could not find category '%1'", category));
            }
        },
        this);

    if (CategoryModel::global()->rootCategories().isEmpty()) {
        connect(CategoryModel::global(), &CategoryModel::rootCategoriesChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverObject::openLocalPackage(const QUrl &localfile)
{
    if (!QFile::exists(localfile.toLocalFile())) {
        // showPassiveNotification(i18n("Trying to open unexisting file '%1'", localfile.toString()));
        qCWarning(DISCOVER_LOG) << "Trying to open unexisting file" << localfile;
        return;
    }
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, localfile]() {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = localfile;
            auto stream = new StoredResultsStream({ResourcesModel::global()->search(f)});
            connect(stream, &StoredResultsStream::finishedResources, this, [this, localfile](const QVector<AbstractResource *> &res) {
                if (res.count() == 1) {
                    Q_EMIT openApplicationInternal(res.first());
                } else {
                    QMimeDatabase db;
                    auto mime = db.mimeTypeForUrl(localfile);
                    auto fIsFlatpakBackend = [](AbstractResourcesBackend *backend) {
                        return backend->metaObject()->className() == QByteArray("FlatpakBackend");
                    };
                    if (mime.name().startsWith(QLatin1String("application/vnd.flatpak"))
                        && !kContains(ResourcesModel::global()->backends(), fIsFlatpakBackend)) {
                        openApplication(QUrl(QStringLiteral("appstream://org.kde.discover.flatpak")));
                        showPassiveNotification(i18n("Cannot interact with flatpak resources without the flatpak backend %1. Please install it first.",
                                                     localfile.toDisplayString()));
                    } else {
                        openMode(QStringLiteral("Browsing"));
                        showPassiveNotification(i18n("Could not open %1", localfile.toDisplayString()));
                    }
                }
            });
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverObject::openApplication(const QUrl &url)
{
    Q_ASSERT(!url.isEmpty());
    showLoadingPage();
    auto action = new OneTimeAction(
        [this, url]() {
            AbstractResourcesBackend::Filters f;
            f.resourceUrl = url;
            auto stream = new StoredResultsStream({ResourcesModel::global()->search(f)});
            connect(stream, &StoredResultsStream::finishedResources, this, [this, url](const QVector<AbstractResource *> &res) {
                if (res.count() >= 1) {
                    Q_EMIT openApplicationInternal(res.first());
                } else if (url.scheme() == QLatin1String("snap")) {
                    openApplication(QUrl(QStringLiteral("appstream://org.kde.discover.snap")));
                    showPassiveNotification(i18n("Please make sure Snap support is installed"));
                } else {
                    Q_EMIT openErrorPage(
                        i18n("Could not open %1 because it was not found in any "
                             "available software repositories. Please report this "
                             "issue to the packagers of your distribution.",
                             url.toDisplayString()));
                }
            });
        },
        this);

    if (ResourcesModel::global()->backends().isEmpty()) {
        connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, action, &OneTimeAction::trigger);
    } else {
        action->trigger();
    }
}

void DiscoverObject::integrateObject(QObject *object)
{
    if (!object) {
        qCWarning(DISCOVER_LOG) << "Errors when loading the GUI";
        QTimer::singleShot(0, QCoreApplication::instance(), []() {
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
    connect(rootObject(), &QQuickView::sceneGraphError, this, [](QQuickWindow::SceneGraphError /*error*/, const QString &message) {
        KCrash::setErrorMessage(message);
        qFatal("%s", qPrintable(message));
    });

    object->installEventFilter(this);
    connect(object, &QObject::destroyed, qGuiApp, &QCoreApplication::quit);

    object->setParent(m_engine);
    connect(qGuiApp, &QGuiApplication::commitDataRequest, this, [this](QSessionManager &sessionManager) {
        if (ResourcesModel::global()->isBusy()) {
            Q_EMIT preventedClose();
            sessionManager.cancel();
        }
    });
}

bool DiscoverObject::eventFilter(QObject *object, QEvent *event)
{
    if (object != rootObject())
        return false;

    if (event->type() == QEvent::Close) {
        if (ResourcesModel::global()->isBusy()) {
            qCWarning(DISCOVER_LOG) << "not closing because there's still pending tasks";
            Q_EMIT preventedClose();
            return true;
        }

        KConfigGroup window(KSharedConfig::openConfig(), "Window");
        window.writeEntry("geometry", rootObject()->geometry());
        window.writeEntry<int>("visibility", rootObject()->visibility());
        // } else if (event->type() == QEvent::ShortcutOverride) {
        //     qCWarning(DISCOVER_LOG) << "Action conflict" << event;
    }
    return false;
}

QString DiscoverObject::iconName(const QIcon &icon)
{
    return icon.name();
}

void DiscoverObject::switchApplicationLanguage()
{
    // auto langDialog = new KSwitchLanguageDialog(nullptr);
    // connect(langDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
    // langDialog->show();
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
    DiscoverTestExecutor(QObject *rootObject, QQmlEngine *engine, const QUrl &url)
        : QObject(engine)
    {
        connect(engine, &QQmlEngine::quit, this, &DiscoverTestExecutor::finish, Qt::QueuedConnection);

        QQmlComponent *component = new QQmlComponent(engine, url, engine);
        m_testObject = component->create(engine->rootContext());

        if (!m_testObject) {
            qCWarning(DISCOVER_LOG) << "error loading test" << url << m_testObject << component->errors();
            Q_ASSERT(false);
        }

        m_testObject->setProperty("appRoot", QVariant::fromValue<QObject *>(rootObject));
        connect(engine, &QQmlEngine::warnings, this, &DiscoverTestExecutor::processWarnings);
    }

    void processWarnings(const QList<QQmlError> &warnings)
    {
        for (const QQmlError &warning : warnings) {
            if (warning.url().path().endsWith(QLatin1String("DiscoverTest.qml"))) {
                qCWarning(DISCOVER_LOG) << "Test failed!" << warnings;
                qGuiApp->exit(1);
            }
        }
        m_warnings << warnings;
    }

    void finish()
    {
        // The CI doesn't seem to have icons, remove when it's not an issue anymore
        m_warnings.erase(std::remove_if(m_warnings.begin(), m_warnings.end(), [](const QQmlError &err) -> bool {
            return err.description().contains(QLatin1String("QML Image: Failed to get image from provider: image://icon/"));
        }));

        if (m_warnings.isEmpty())
            qCDebug(DISCOVER_LOG) << "cool no warnings!";
        else
            qCDebug(DISCOVER_LOG) << "test finished successfully despite" << m_warnings;
        qGuiApp->exit(m_warnings.count());
    }

private:
    QObject *m_testObject;
    QList<QQmlError> m_warnings;
};

void DiscoverObject::loadTest(const QUrl &url)
{
    new DiscoverTestExecutor(rootObject(), engine(), url);
}

QQuickWindow *DiscoverObject::rootObject() const
{
    return qobject_cast<QQuickWindow *>(m_engine->rootObjects().at(0));
}

void DiscoverObject::showPassiveNotification(const QString &msg)
{
    QTimer::singleShot(100, this, [this, msg]() {
        QMetaObject::invokeMethod(rootObject(),
                                  "showPassiveNotification",
                                  Qt::QueuedConnection,
                                  Q_ARG(QVariant, msg),
                                  Q_ARG(QVariant, {}),
                                  Q_ARG(QVariant, {}),
                                  Q_ARG(QVariant, {}));
    });
}

void DiscoverObject::copyTextToClipboard(const QString text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void DiscoverObject::reboot()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptReboot"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverObject::rebootNow()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"),
                                                 QStringLiteral("/org/freedesktop/login1"),
                                                 QStringLiteral("org.freedesktop.login1.Manager"),
                                                 QStringLiteral("Reboot"));
    method.setArguments({true /*interactive*/});
    QDBusConnection::systemBus().asyncCall(method);
}

QRect DiscoverObject::initialGeometry() const
{
    KConfigGroup window(KSharedConfig::openConfig(), "Window");
    return window.readEntry("geometry", QRect());
}

QString DiscoverObject::describeSources() const
{
    return rootObject()->property("describeSources").toString();
}

#include "DiscoverObject.moc"
