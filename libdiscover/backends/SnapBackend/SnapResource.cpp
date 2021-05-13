/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SnapResource.h"
#include "SnapBackend.h"
#include <KLocalizedString>
#include <QBuffer>
#include <QDebug>
#include <QImageReader>
#include <QProcess>
#include <QStandardItemModel>

#ifdef SNAP_MARKDOWN
#include <Snapd/MarkdownParser>
#endif

#include <utils.h>

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

const QStringList SnapResource::m_objects({QStringLiteral("qrc:/qml/PermissionsButton.qml")
#ifdef SNAP_CHANNELS
                                               ,
                                           QStringLiteral("qrc:/qml/ChannelsButton.qml")
#endif
});

SnapResource::SnapResource(QSharedPointer<QSnapdSnap> snap, AbstractResource::State state, SnapBackend *backend)
    : AbstractResource(backend)
    , m_state(state)
    , m_snap(snap)
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

int SnapResource::size()
{
    // return isInstalled() ? m_snap->installedSize() : m_snap->downloadSize();
    return m_snap->downloadSize();
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
    return {QJsonObject{{QStringLiteral("name"), m_snap->license()}}};
}

#ifdef SNAP_MARKDOWN
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
#endif

QString SnapResource::longDescription()
{
#ifdef SNAP_MARKDOWN
    QSnapdMarkdownParser parser(QSnapdMarkdownParser::MarkdownVersion0);
    QList<QSnapdMarkdownNode> nodes = parser.parse(m_snap->description());
    QString result;
    for (int i = 0; i < nodes.size(); i++)
        result += serialize_node(nodes[i]);
    return result;
#else
    return m_snap->description();
#endif
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
    QList<QUrl> screenshots;
#ifdef SNAP_MEDIA
    for (int i = 0, c = m_snap->mediaCount(); i < c; ++i) {
        QScopedPointer<QSnapdMedia> media(m_snap->media(i));
        if (media->type() == QLatin1String("screenshot"))
            screenshots << QUrl(media->url());
    }
#else
    for (int i = 0, c = m_snap->screenshotCount(); i < c; ++i) {
        QScopedPointer<QSnapdScreenshot> screenshot(m_snap->screenshot(i));
        screenshots << QUrl(screenshot->url());
    }
#endif
    Q_EMIT screenshotsFetched(screenshots, screenshots);
}

void SnapResource::invokeApplication() const
{
    QProcess::startDetached(QStringLiteral("snap"), {QStringLiteral("run"), packageName()});
}

AbstractResource::Type SnapResource::type() const
{
    return m_snap->snapType() != QLatin1String("app") ? Application : Technical;
}

void SnapResource::setSnap(const QSharedPointer<QSnapdSnap> &snap)
{
    Q_ASSERT(snap->name() == m_snap->name());
    if (m_snap == snap)
        return;

    const bool newSize = m_snap->installedSize() != snap->installedSize() || m_snap->downloadSize() != snap->downloadSize();
    m_snap = snap;
    if (newSize)
        Q_EMIT sizeChanged();

    Q_EMIT newSnap();
}

QDate SnapResource::releaseDate() const
{
    return {};
}

class PlugsModel : public QStandardItemModel
{
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
                    item->setCheckState(plug->connectionCount() > 0 ? Qt::Checked : Qt::Unchecked);
                    item->setData(plug->name(), PlugNameRole);
                    item->setData(slot->snap(), SlotSnapRole);
                    item->setData(slot->name(), SlotNameRole);
                    appendRow(item);
                }
            }
        }
    }

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
            Q_EMIT m_res->backend()->passiveMessage(req->errorString());
        }
        return req->error() == QSnapdRequest::NoError;
    }

    SnapResource *const m_res;
    SnapBackend *const m_backend;
};

QAbstractItemModel *SnapResource::plugs(QObject *p)
{
    if (!isInstalled())
        return new QStandardItemModel(p);

    return new PlugsModel(this, qobject_cast<SnapBackend *>(parent()), p);
}

QString SnapResource::appstreamId() const
{
    const QStringList ids
#if defined(SNAP_COMMON_IDS)
        = m_snap->commonIds()
#endif
        ;
    return ids.isEmpty() ? QLatin1String("io.snapcraft.") + m_snap->name() + QLatin1Char('-') + m_snap->id() : ids.first();
}

QString SnapResource::channel() const
{
#ifdef SNAP_PUBLISHER
    auto req = client()->getSnap(packageName());
#else
    auto req = client()->listOne(packageName());
#endif
    req->runSync();
    return req->error() ? QString() : req->snap()->trackingChannel();
}

QString SnapResource::author() const
{
#ifdef SNAP_PUBLISHER
    QString author = m_snap->publisherDisplayName();
    if (m_snap->publisherValidation() == QSnapdEnums::PublisherValidationVerified) {
        author += QStringLiteral(" ✅");
    }
#else
    QString author;
#endif

    return author;
}

void SnapResource::setChannel(const QString &channelName)
{
#ifdef SNAP_CHANNELS
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
#endif
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

#ifdef SNAP_CHANNELS
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
        for (int i = 0, c = s->channelCount(); i < c; ++i) {
            auto channel = s->channel(i);
            channel->setParent(this);
            m_channels << channel;
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

#endif

QObject *SnapResource::channels(QObject *parent)
{
#ifdef SNAP_CHANNELS
    return new Channels(this, parent);
#else
    return nullptr;
#endif
}

#include "SnapResource.moc"
