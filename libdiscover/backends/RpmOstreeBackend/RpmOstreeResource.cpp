#include "RpmOstreeResource.h"
#include "RpmOstreeBackend.h"
#include <QStandardItemModel>

const QStringList RpmOstreeResource::m_objects({QStringLiteral("qrc:/qml/RemoteRefsButton.qml")});

RpmOstreeResource::RpmOstreeResource(QString name,
                                     QString baseVersion,
                                     QString checkSum,
                                     QString signature,
                                     QString layeredPackages,
                                     QString localPackages,
                                     QString origin,
                                     RpmOstreeBackend *parent)
    : AbstractResource(parent)
    , m_deploymentName(name)
    , m_version(baseVersion)
    , m_checkSum(checkSum)
    , m_signature(signature)
    , m_layeredPackages(layeredPackages)
    , m_localPackages(localPackages)
    , m_currentRefs(origin)
    , m_state(AbstractResource::None)
{
    connect(this, &RpmOstreeResource::buttonPressed, parent, &RpmOstreeBackend::perfromSystemUpgrade);
}

void RpmOstreeResource::setRemoteRefsList(QStringList remoteRefs)
{
    if (!m_remoteRefsList.isEmpty())
        m_remoteRefsList.clear();
    m_remoteRefsList = remoteRefs;
}

QString RpmOstreeResource::getRecentRemoteRefs()
{
    if (!isRecentRefsAvaliable())
        return QStringLiteral("");
    QString recentRefs = m_recentRefs;
    QStringList str = recentRefs.split(QStringLiteral("/"));
    QString refs = QStringLiteral("Kinoite ") + str[1];
    return refs;
}

bool RpmOstreeResource::isRecentRefsAvaliable()
{
    QString currentRefsVersion = m_currentRefs;
    QStringList str = currentRefsVersion.split(QStringLiteral("/"));
    int currentVersion = str[1].toInt();

    for (const QString &refs : m_remoteRefsList) {
        if (refs == m_currentRefs)
            continue;
        QString refssV = refs;
        QStringList refsNumber = refssV.split(QStringLiteral("/"));
        int refsNumberV = refsNumber[1].toInt();
        if (refsNumberV <= currentVersion)
            continue;
        m_recentRefs = refs;
    }

    if (m_recentRefs.isEmpty())
        return false;
    return true;
}

QString RpmOstreeResource::availableVersion() const
{
    return m_newVersion;
}

void RpmOstreeResource::setNewVersion(QString newVersion)
{
    m_newVersion = newVersion;
}

QString RpmOstreeResource::appstreamId() const
{
    return QStringLiteral(" ");
}

bool RpmOstreeResource::canExecute() const
{
    return true;
}

QVariant RpmOstreeResource::icon() const
{
    static const QVector<QVariant> icons = {QStringLiteral("device-notifier"), QStringLiteral("media-floppy"), QStringLiteral("drink-beer")};
    return icons[type()];
}

QString RpmOstreeResource::installedVersion() const
{
    return m_version;
}

QUrl RpmOstreeResource::url() const
{
    return QUrl();
}

QUrl RpmOstreeResource::donationURL()
{
    return QUrl();
}

QUrl RpmOstreeResource::homepage()
{
    return QUrl();
}

QJsonArray RpmOstreeResource::licenses()
{
    return {QJsonObject{{QStringLiteral("name"), QString()}, {QStringLiteral("url"), QString()}}};
}

QString RpmOstreeResource::longDescription()
{
    QString description = QStringLiteral("Version: ") + name() + installedVersion() + QStringLiteral("\n") + QStringLiteral("BaseCommit: ") + m_checkSum
        + QStringLiteral("\n") + QStringLiteral("GPGSignature: Valid signature by ") + m_signature + QStringLiteral("\n") + QStringLiteral("Origin: ")
        + m_currentRefs + QStringLiteral("\n");
    return description;
}

QString RpmOstreeResource::name() const
{
    return m_deploymentName;
}

QString RpmOstreeResource::origin() const
{
    return QStringLiteral("RPM-OSTree");
}

QString RpmOstreeResource::packageName() const
{
    return QStringLiteral(" ");
}

QString RpmOstreeResource::section()
{
    return QString();
}

AbstractResource::State RpmOstreeResource::state()
{
    return m_state;
}

void RpmOstreeResource::invokeApplication() const
{
    qWarning() << " OSTreeRPMResource::invokeApplication ";
}

void RpmOstreeResource::fetchChangelog()
{
    qWarning() << "OSTreeRPMResource::fetchChangelog";
}

void RpmOstreeResource::fetchScreenshots()
{
    qWarning() << "OSTreeRPMResource::fetchScreenshots";
}

QString RpmOstreeResource::author() const
{
    return QStringLiteral("");
}

QString RpmOstreeResource::comment()
{
    QString comment = QStringLiteral("The current active deployment.");
    return comment;
}

int RpmOstreeResource::size()
{
    return 0;
}

QDate RpmOstreeResource::releaseDate() const
{
    return {};
}

QString RpmOstreeResource::executeLabel() const
{
    return QStringLiteral("Launch");
}

void RpmOstreeResource::setState(AbstractResource::State state)
{
    m_state = state;
    emit stateChanged();
}

void RpmOstreeResource::rebaseToNewVersion()
{
    Q_EMIT buttonPressed(m_recentRefs);
}