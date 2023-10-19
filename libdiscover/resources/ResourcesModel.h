/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QList>
#include <QSet>
#include <QTimer>

#include "AbstractResourcesBackend.h"
#include "discovercommon_export.h"

class DiscoverAction;

class DISCOVERCOMMON_EXPORT AggregatedResultsStream : public ResultsStream
{
    Q_OBJECT
public:
    AggregatedResultsStream(const QSet<ResultsStream *> &streams);
    ~AggregatedResultsStream();

    QSet<QObject *> streams() const
    {
        return m_streams;
    }

Q_SIGNALS:
    void finished();

private:
    void addResults(const QList<StreamResult> &res);
    void emitResults();
    void streamDestruction(QObject *obj);
    void resourceDestruction(QObject *obj);
    void clear();

    QSet<QObject *> m_streams;
    QList<StreamResult> m_results;
    QTimer m_delayedEmission;
};

template<typename T>
class EmitWhenChanged
{
public:
    EmitWhenChanged(T initial, const std::function<T()> &get, const std::function<void(T)> &emitChanged)
        : m_get(get)
        , m_emitChanged(emitChanged)
        , m_value(initial)
    {
    }

    void reevaluate()
    {
        auto newValue = m_get();
        if (newValue != m_value) {
            m_value = newValue;
            m_emitChanged(m_value);
        }
    }

    std::function<T()> const m_get;
    std::function<void(T)> const m_emitChanged;
    T m_value;
};

class DISCOVERCOMMON_EXPORT ResourcesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool hasSecurityUpdates READ hasSecurityUpdates NOTIFY updatesCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY fetchingChanged)
    Q_PROPERTY(bool isInitializing READ isInitializing NOTIFY allInitialized)
    Q_PROPERTY(AbstractResourcesBackend *currentApplicationBackend READ currentApplicationBackend WRITE setCurrentApplicationBackend NOTIFY
                   currentApplicationBackendChanged)
    Q_PROPERTY(DiscoverAction *updateAction READ updateAction CONSTANT)
    Q_PROPERTY(int fetchingUpdatesProgress READ fetchingUpdatesProgress NOTIFY fetchingUpdatesProgressChanged)
    Q_PROPERTY(QString applicationSourceName READ applicationSourceName NOTIFY currentApplicationBackendChanged)
    Q_PROPERTY(InlineMessage *inlineMessage READ inlineMessage NOTIFY inlineMessageChanged)
    Q_PROPERTY(QString distroName READ distroName CONSTANT)
public:
    /** This constructor should be only used by unit tests.
     *  @p backendName defines what backend will be loaded when the backend is constructed.
     */
    explicit ResourcesModel(const QString &backendName, QObject *parent = nullptr);
    static ResourcesModel *global();
    ~ResourcesModel() override;

    QList<AbstractResourcesBackend *> backends() const;
    int updatesCount() const
    {
        return m_updatesCount.m_value;
    }
    bool hasSecurityUpdates() const;

    bool isBusy() const;
    bool isFetching() const;
    bool isInitializing() const;

    Q_SCRIPTABLE bool isExtended(const QString &id);

    AggregatedResultsStream *search(const AbstractResourcesBackend::Filters &search);
    void checkForUpdates();

    QString applicationSourceName() const;

    void setCurrentApplicationBackend(AbstractResourcesBackend *backend, bool writeConfig = true);
    AbstractResourcesBackend *currentApplicationBackend() const;

    DiscoverAction *updateAction() const
    {
        return m_updateAction;
    }
    int fetchingUpdatesProgress() const
    {
        return m_fetchingUpdatesProgress.m_value;
    }

    QString distroName() const;
    Q_INVOKABLE QUrl distroBugReportUrl();

    void setInlineMessage(const QSharedPointer<InlineMessage> &inlineMessage);
    InlineMessage *inlineMessage() const
    {
        return m_inlineMessage.data();
    }

public Q_SLOTS:
    void installApplication(AbstractResource *app, const AddonList &addons);
    void installApplication(AbstractResource *app);
    void removeApplication(AbstractResource *app);

Q_SIGNALS:
    void fetchingChanged(bool isFetching);
    void allInitialized();
    void backendsChanged();
    void updatesCountChanged(int updatesCount);
    void backendDataChanged(AbstractResourcesBackend *backend, const QList<QByteArray> &properties);
    void resourceDataChanged(AbstractResource *resource, const QList<QByteArray> &properties);
    void resourceRemoved(AbstractResource *resource);
    void passiveMessage(const QString &message);
    void currentApplicationBackendChanged(AbstractResourcesBackend *currentApplicationBackend);
    void fetchingUpdatesProgressChanged(int fetchingUpdatesProgress);
    void inlineMessageChanged(const QSharedPointer<InlineMessage> &inlineMessage);
    void switchToUpdates();

private Q_SLOTS:
    void callerFetchingChanged();
    void updateCaller(const QList<QByteArray> &properties);
    void registerAllBackends();

private:
    ///@p initialize tells if all backends load will be triggered on construction
    explicit ResourcesModel(QObject *parent = nullptr);
    void init(bool load);
    void addResourcesBackend(AbstractResourcesBackend *backend);
    void registerBackendByName(const QString &name);
    void initApplicationsBackend();
    void slotFetching();

    bool m_isFetching;
    bool m_isInitializing = true;
    QList<AbstractResourcesBackend *> m_backends;
    int m_initializingBackendsCount;
    DiscoverAction *m_updateAction = nullptr;
    AbstractResourcesBackend *m_currentApplicationBackend;
    QTimer *m_allInitializedEmitter;

    EmitWhenChanged<int> m_updatesCount;
    EmitWhenChanged<int> m_fetchingUpdatesProgress;
    QSharedPointer<InlineMessage> m_inlineMessage;

    static ResourcesModel *s_self;
};
