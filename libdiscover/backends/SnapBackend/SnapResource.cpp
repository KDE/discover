/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SnapResource.h"
#include "SnapBackend.h"
#include "libdiscover_snap_debug.h"
#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <KLocalizedString>
#include <QBuffer>
#include <QDebug>
#include <QDBusInterface>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QProcess>
#include <QStandardItemModel>
#include <Snapd/MarkdownParser>

#include <appstream/AppStreamUtils.h>
#include <utils.h>

using namespace Qt::StringLiterals;

QDebug operator<<(QDebug debug, const QSnapdPlug &plug)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QSnapdPlug(";
    debug.nospace() << "name:" << plug.name() << ',';
    debug.nospace() << "snap:" << plug.snap() << ',';
    debug.nospace() << "label:" << plug.label() << ',';
    debug.nospace() << "interface:" << plug.interface() << ',';
    // debug.nospace() << "connectionCount:" << plug.connectionSlotCount();
    debug.nospace() << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const QSnapdSlot &slot)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QSnapdSlot(";
    debug.nospace() << "name:" << slot.name() << ',';
    debug.nospace() << "label:" << slot.label() << ',';
    debug.nospace() << "snap:" << slot.snap() << ',';
    debug.nospace() << "interface:" << slot.interface() << ',';
    // debug.nospace() << "connectionCount:" << slot.connectionSlotCount();
    debug.nospace() << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const QSnapdPlug *plug)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "*" << *plug;
    return debug;
}

QDebug operator<<(QDebug debug, const QSnapdSlot *slot)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "*" << *slot;
    return debug;
}

const QStringList SnapResource::s_topObjects({
    QStringLiteral("qrc:/qml/PermissionsButton.qml"),
    QStringLiteral("qrc:/qml/ChannelsButton.qml"),
});

SnapResource::SnapResource(QSharedPointer<QSnapdSnap> snap, AbstractResource::State state, SnapBackend *backend)
    : AbstractResource(backend)
    , m_state(state)
    , m_installedSize(0)
    , m_downloadSize(0)
    , m_snap(snap)
    , m_channel(QStringLiteral("latest/stable"))
{
    setObjectName(snap->name());
}

QSnapdClient *SnapResource::client() const
{
    auto backend = qobject_cast<SnapBackend *>(parent());
    return backend->client();
}

QString SnapResource::availableVersion() const
{
    return installedVersion();
}

QStringList SnapResource::categories()
{
    return {QStringLiteral("Application")};
}

QString SnapResource::comment()
{
    return m_snap->summary();
}

quint64 SnapResource::size()
{
    if (m_state == AbstractResource::Installed) {
        return installedSize();
    } else {
        return downloadSize();
    }
}

QVariant SnapResource::icon() const
{
    if (m_icon.isNull()) {
        m_icon = [this]() -> QVariant {
            const auto iconPath = m_snap->icon();
            if (iconPath.isEmpty())
                return QStringLiteral("package-x-generic");

            if (!iconPath.startsWith(QLatin1Char('/')))
                return QUrl(iconPath);

            auto req = client()->getIcon(packageName());
            connect(req, &QSnapdGetIconRequest::complete, this, &SnapResource::gotIcon);
            req->runAsync();
            return {};
        }();
    }
    return m_icon;
}

void SnapResource::gotIcon()
{
    auto req = qobject_cast<QSnapdGetIconRequest *>(sender());
    if (req->error()) {
        qWarning() << "icon error" << req->errorString();
        return;
    }

    auto icon = req->icon();

    QBuffer buffer;
    buffer.setData(icon->data());
    QImageReader reader(&buffer);

    auto theIcon = QVariant::fromValue<QImage>(reader.read());
    if (theIcon != m_icon) {
        m_icon = theIcon;
        Q_EMIT iconChanged();
    }
}

QString SnapResource::installedVersion() const
{
    return m_snap->version();
}

QJsonArray SnapResource::licenses()
{
    return AppStreamUtils::licenses(m_snap->license());
}

static QString serialize_node(QSnapdMarkdownNode &node);

static QString serialize_children(QSnapdMarkdownNode &node)
{
    QString result;
    for (int i = 0; i < node.childCount(); i++) {
        QScopedPointer<QSnapdMarkdownNode> child(node.child(i));
        result += serialize_node(*child);
    }
    return result;
}

static QString serialize_node(QSnapdMarkdownNode &node)
{
    switch (node.type()) {
    case QSnapdMarkdownNode::NodeTypeText:
        return node.text().toHtmlEscaped();

    case QSnapdMarkdownNode::NodeTypeParagraph:
        return QLatin1String("<p>") + serialize_children(node) + QLatin1String("</p>\n");

    case QSnapdMarkdownNode::NodeTypeUnorderedList:
        return QLatin1String("<ul>\n") + serialize_children(node) + QLatin1String("</ul>\n");

    case QSnapdMarkdownNode::NodeTypeListItem:
        if (node.childCount() == 0)
            return QLatin1String("<li></li>\n");
        if (node.childCount() == 1) {
            QScopedPointer<QSnapdMarkdownNode> child(node.child(0));
            if (child->type() == QSnapdMarkdownNode::NodeTypeParagraph)
                return QLatin1String("<li>") + serialize_children(*child) + QLatin1String("</li>\n");
        }
        return QLatin1String("<li>\n") + serialize_children(node) + QLatin1String("</li>\n");

    case QSnapdMarkdownNode::NodeTypeCodeBlock:
        return QLatin1String("<pre><code>") + serialize_children(node) + QLatin1String("</code></pre>\n");

    case QSnapdMarkdownNode::NodeTypeCodeSpan:
        return QLatin1String("<code>") + serialize_children(node) + QLatin1String("</code>");

    case QSnapdMarkdownNode::NodeTypeEmphasis:
        return QLatin1String("<em>") + serialize_children(node) + QLatin1String("</em>");

    case QSnapdMarkdownNode::NodeTypeStrongEmphasis:
        return QLatin1String("<strong>") + serialize_children(node) + QLatin1String("</strong>");

    case QSnapdMarkdownNode::NodeTypeUrl:
        return serialize_children(node);

    default:
        return QString();
    }
}

QString SnapResource::longDescription()
{
    QSnapdMarkdownParser parser(QSnapdMarkdownParser::MarkdownVersion0);
    QList<QSnapdMarkdownNode> nodes = parser.parse(m_snap->description());
    QString result;
    for (int i = 0; i < nodes.size(); i++)
        result += serialize_node(nodes[i]);
    return result;
}

QString SnapResource::name() const
{
    return m_snap->title().isEmpty() ? m_snap->name() : m_snap->title();
}

QString SnapResource::origin() const
{
    return QStringLiteral("Snap");
}

QString SnapResource::packageName() const
{
    return m_snap->name();
}

QString SnapResource::section()
{
    return QStringLiteral("snap");
}

AbstractResource::State SnapResource::state()
{
    return m_state;
}

void SnapResource::setState(AbstractResource::State state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void SnapResource::fetchChangelog()
{
    QString log;
    Q_EMIT changelogFetched(log);
}

void SnapResource::fetchScreenshots()
{
    Screenshots screenshots;
    for (int i = 0, c = m_snap->mediaCount(); i < c; ++i) {
        QScopedPointer<QSnapdMedia> media(m_snap->media(i));
        if (media->type() == QLatin1String("screenshot"))
            screenshots << QUrl(media->url());
    }
    Q_EMIT screenshotsFetched(screenshots);
}

QString SnapResource::launchableDesktopFile() const
{
    QString desktopFile;
    qCDebug(LIBDISCOVER_SNAP_LOG) << "Snap: " << packageName() << " - " << m_snap->appCount() << " app(s) detected";
    for (int i = 0; i < m_snap->appCount(); i++) {
        const auto appName = m_snap->app(i)->name();
        const auto appDesktopFile = m_snap->app(i)->desktopFile();
        if (appDesktopFile.isEmpty()) {
            qCDebug(LIBDISCOVER_SNAP_LOG) << "App " << i << ": " << appName << ": " << "No desktop file, skipping";
            continue;
        }
        if (packageName().compare(appName, Qt::CaseInsensitive) == 0) {
            qCDebug(LIBDISCOVER_SNAP_LOG) << "App " << i << ": " << appName << ": " << "Main app, stopping search";
            desktopFile = appDesktopFile;
            break;
        }
        if (desktopFile.isEmpty()) {
            qCDebug(LIBDISCOVER_SNAP_LOG) << "App " << i << ": " << appName << ": " << "First candidate, keeping for now";
            desktopFile = appDesktopFile;
        }
    }
    if (desktopFile.isEmpty()) {
        qCWarning(LIBDISCOVER_SNAP_LOG) << "No desktop file found for this snap, trying expected name for the main app desktop file";
        desktopFile = packageName() + QLatin1Char('_') + packageName() + QLatin1StringView(".desktop");
    }
    return QFileInfo(desktopFile).fileName();
}

void SnapResource::invokeApplication() const
{
    QDBusInterface interface(
            QStringLiteral("io.snapcraft.Launcher"),
            QStringLiteral("/io/snapcraft/PrivilegedDesktopLauncher"),
            QStringLiteral("io.snapcraft.PrivilegedDesktopLauncher"),
            QDBusConnection::sessionBus()
    );
    interface.call(QStringLiteral("OpenDesktopEntry"), launchableDesktopFile());
}

AbstractResource::Type SnapResource::type() const
{
    switch (m_snap->snapType()) {
    case QSnapdEnums::SnapTypeApp:
        return Application;
    case QSnapdEnums::SnapTypeCore:
    case QSnapdEnums::SnapTypeBase:
        return ApplicationSupport;
    case QSnapdEnums::SnapTypeGadget:
    case QSnapdEnums::SnapTypeKernel:
    case QSnapdEnums::SnapTypeOperatingSystem:
    case QSnapdEnums::SnapTypeSnapd:
    case QSnapdEnums::SnapTypeUnknown:
        return System;
    }
    return System;
}

void SnapResource::setSnap(const QSharedPointer<QSnapdSnap> &snap)
{
    Q_ASSERT(snap->name() == m_snap->name());
    if (m_snap == snap)
        return;

    const auto oldSize = size();
    m_snap = snap;
    updateSizes();
    const auto newSize = size();

    if (newSize != oldSize)
        Q_EMIT sizeChanged();

    Q_EMIT newSnap();
}

QDate SnapResource::releaseDate() const
{
    return {};
}

class PlugsModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum Roles {
        PlugNameRole = Qt::UserRole + 1,
        SlotSnapRole,
        SlotNameRole,
    };

    PlugsModel(SnapResource *res, SnapBackend *backend, QObject *parent)
        : QStandardItemModel(parent)
        , m_res(res)
        , m_backend(backend)
    {
        auto roles = roleNames();
        roles.insert(Qt::CheckStateRole, "checked");
        setItemRoleNames(roles);

        auto req = backend->client()->getInterfaces();
        req->runSync();

        QHash<QString, QVector<QSnapdSlot *>> slotsForInterface;
        for (int i = 0; i < req->slotCount(); ++i) {
            const auto slot = req->slot(i);
            slot->setParent(this);
            slotsForInterface[slot->interface()].append(slot);
        }

        const auto snap = m_res->snap();
        for (int i = 0; i < req->plugCount(); ++i) {
            const QScopedPointer<QSnapdPlug> plug(req->plug(i));
            if (plug->snap() == snap->name()) {
                if (plug->interface() == QLatin1String("content"))
                    continue;

                const auto theSlots = slotsForInterface.value(plug->interface());
                for (auto slot : theSlots) {
                    auto item = new QStandardItem;
                    if (plug->label().isEmpty())
                        item->setText(plug->name());
                    else
                        item->setText(i18n("%1 - %2", plug->name(), plug->label()));

                    // qDebug() << "xxx" << plug->name() << plug->label() << plug->interface() << slot->snap() << "slot:" << slot->name() <<
                    // slot->snap() << slot->interface() << slot->label();
                    item->setCheckable(true);
                    item->setCheckState(plug->connectedSlotCount() > 0 ? Qt::Checked : Qt::Unchecked);
                    item->setData(plug->name(), PlugNameRole);
                    item->setData(slot->snap(), SlotSnapRole);
                    item->setData(slot->name(), SlotNameRole);
                    appendRow(item);
                }
            }
        }
    }

Q_SIGNALS:
    void error(InlineMessage *message);

private:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::CheckStateRole)
            return QStandardItemModel::setData(index, value, role);

        auto item = itemFromIndex(index);
        Q_ASSERT(item);
        const QString plugName = item->data(PlugNameRole).toString();
        const QString slotSnap = item->data(SlotSnapRole).toString();
        const QString slotName = item->data(SlotNameRole).toString();

        QSnapdRequest *req;

        const auto snap = m_res->snap();
        if (item->checkState() == Qt::Checked) {
            req = m_backend->client()->disconnectInterface(snap->name(), plugName, slotSnap, slotName);
        } else {
            req = m_backend->client()->connectInterface(snap->name(), plugName, slotSnap, slotName);
        }
        req->runSync();
        if (req->error()) {
            qWarning() << "snapd error" << req->errorString();
            Q_EMIT error(new InlineMessage(InlineMessage::Error, u"error"_s, req->errorString()));
        }
        return req->error() == QSnapdRequest::NoError;
    }

    SnapResource *const m_res;
    SnapBackend *const m_backend;
};

QAbstractItemModel *SnapResource::plugs(QObject *p)
{
    if (!isInstalled())
        return nullptr;

    return new PlugsModel(this, qobject_cast<SnapBackend *>(parent()), p);
}

QString SnapResource::appstreamId() const
{
    const QStringList ids = m_snap->commonIds();
    return ids.isEmpty() ? QLatin1String("io.snapcraft.") + m_snap->name() + QLatin1Char('-') + m_snap->id() : ids.first();
}

QStringList SnapResource::topObjects() const
{
    return s_topObjects;
}

QString SnapResource::channel()
{
    if (isInstalled()) {
        auto req = client()->getSnap(packageName());
        req->runSync();
        return req->error() ? QString() : req->snap()->trackingChannel();
    }
    return m_channel;
}

QString SnapResource::author() const
{
    QString author = m_snap->publisherDisplayName();
    if (m_snap->publisherValidation() == QSnapdEnums::PublisherValidationVerified) {
        author += QStringLiteral(" ✅");
    }
    return author;
}

void SnapResource::setChannel(const QString &channelName)
{
    if (!isInstalled()) {
        m_channel = channelName;
        Q_EMIT channelChanged(channelName);
        return;
    }
    Q_ASSERT(isInstalled());
    auto request = client()->switchChannel(m_snap->name(), channelName);

    const auto currentChannel = channel();
    request->runAsync();
    connect(request, &QSnapdRequest::complete, this, [this, currentChannel]() {
        const auto newChannel = channel();
        if (newChannel != currentChannel) {
            Q_EMIT channelChanged(newChannel);
        }
    });
}

quint64 SnapResource::installedSize() const
{
    return m_installedSize;
}

quint64 SnapResource::downloadSize() const
{
    return m_downloadSize;
}

void SnapResource::updateSizes()
{
    if (m_snap->installedSize() > 0) {
        m_installedSize = m_snap->installedSize();
    }

    if (m_snap->downloadSize() > 0) {
        m_downloadSize = m_snap->downloadSize();
    }
}

void SnapResource::refreshSnap()
{
    auto request = client()->find(QSnapdClient::FindFlag::MatchName, m_snap->name());
    connect(request, &QSnapdRequest::complete, this, [this, request]() {
        if (request->error()) {
            qWarning() << "error" << request->error() << ": " << request->errorString();
            return;
        }
        Q_ASSERT(request->snapCount() == 1);
        setSnap(QSharedPointer<QSnapdSnap>(request->snap(0)));
    });
    request->runAsync();
}

class Channels : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject *> channels READ channels NOTIFY channelsChanged)

public:
    Channels(SnapResource *res, QObject *parent)
        : QObject(parent)
        , m_res(res)
    {
        if (res->snap()->channelCount() == 0)
            res->refreshSnap();
        else
            refreshChannels();

        connect(res, &SnapResource::newSnap, this, &Channels::refreshChannels);
    }

    void refreshChannels()
    {
        qDeleteAll(m_channels);
        m_channels.clear();

        auto s = m_res->snap();
        const auto risks = { QLatin1StringView("stable"), QLatin1StringView("candidate"), QLatin1StringView("beta"), QLatin1StringView("edge")};
        QStringList tempChannels;
        for (auto track : s->tracks()) {
            for (const auto &risk : risks) {
                auto channel = s->matchChannel(track + QLatin1Char('/') + risk);
                if (channel && !tempChannels.contains(channel->name())) {
                    m_channels << channel;
                    tempChannels << channel->name();
                }
            }
        }
        Q_EMIT channelsChanged();
    }

    QList<QObject *> channels() const
    {
        return m_channels;
    }

Q_SIGNALS:
    void channelsChanged();

private:
    QList<QObject *> m_channels;
    SnapResource *const m_res;
};

QObject *SnapResource::channels(QObject *parent)
{
    return new Channels(this, parent);
}

#include "SnapResource.moc"
#include "moc_SnapResource.cpp"
