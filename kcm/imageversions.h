/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KAuth/ExecuteJob>

#include <QAbstractListModel>
#include <QPointer>
#include <QSet>
#include <QString>

class ImageVersions : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool incomplete READ isIncomplete NOTIFY incompleteChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    enum Role {
        VersionRole = Qt::UserRole,
        DateRole,
        PinnedRole,
        /// Protected by configuration we don't own, so not toggleable here.
        EnforcedRole,
        RunningRole,
    };
    Q_ENUM(Role)

    static bool isSupported();

    explicit ImageVersions(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isBusy() const;
    bool isIncomplete() const;
    QString errorMessage() const;

    void load();
    void save();
    void defaults();

    bool isSaveNeeded() const;
    bool isDefaults() const;

    Q_INVOKABLE void setPinned(int row, bool pinned);

Q_SIGNALS:
    void busyChanged();
    void incompleteChanged();
    void errorMessageChanged();

    void configurationChanged();

private:
    struct Version {
        QString version;
        QString date;
        bool pinned = false;
        bool enforced = false;
        bool running = false;
    };

    void queryVersions();
    void rebuild(QSet<QString> installed);
    QSet<QString> pinnedVersions() const;

    void beginOperation();
    void endOperation();
    void setIncomplete(bool incomplete);
    void setError(const QString &message);

    QList<Version> m_versions;
    QSet<QString> m_pinned;
    QSet<QString> m_enforced;
    QSet<QString> m_applied;
    QString m_running;
    QString m_error;
    QPointer<KAuth::ExecuteJob> m_saveJob;
    int m_pendingOperations = 0;
    bool m_incomplete = false;
};
