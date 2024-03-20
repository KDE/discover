/*
 *   SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <PackageKit/Transaction>
#include <QPointer>

#include <optional>
#include <variant>

class PackageKitFetchDependenciesJob;

// Minimal info about a package
class PackageKitDependency
{
    Q_GADGET
    Q_PROPERTY(PackageKit::Transaction::Info info MEMBER m_info CONSTANT FINAL)
    Q_PROPERTY(QString infoString MEMBER m_infoString CONSTANT FINAL)
    Q_PROPERTY(QString packageId MEMBER m_packageId CONSTANT FINAL)
    Q_PROPERTY(QString packageName READ packageName CONSTANT FINAL)
    Q_PROPERTY(QString summary MEMBER m_summary CONSTANT FINAL)

public:
    explicit PackageKitDependency() = default; // for the sake of QVariant
    explicit PackageKitDependency(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);

    bool operator==(const PackageKitDependency &other) const;

    PackageKit::Transaction::Info info() const;
    QString infoString() const;
    QString packageId() const;
    QString packageName() const;
    QString summary() const;

private:
    PackageKit::Transaction::Info m_info;
    QString m_infoString;
    QString m_packageId;
    QString m_summary;
};

// Lazy job runner. Guards against starting two jobs at once. Resets itself when package ID changes.
class PackageKitDependencies : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString packageId READ packageId WRITE setPackageId NOTIFY packageIdChanged)
    Q_PROPERTY(QList<PackageKitDependency> dependencies READ dependencies NOTIFY dependenciesChanged)

public:
    explicit PackageKitDependencies(QObject *parent = nullptr);
    ~PackageKitDependencies() override;

    QString packageId() const;
    void setPackageId(const QString &packageId);

    QList<PackageKitDependency> dependencies();

    void refresh();

Q_SIGNALS:
    void packageIdChanged();
    void dependenciesChanged();

private Q_SLOTS:
    void onJobFinished(QList<PackageKitDependency> dependencies);

private:
    using Job = QPointer<PackageKitFetchDependenciesJob>;
    using Data = QList<PackageKitDependency>;

    void cancel(bool notify);

    QString m_packageId;
    // tri-state:
    // - no work done or even started yet: nullopt;
    // - job is currently running, no need to start another one: optional(Job);
    // - data is available: optional(Data).
    std::optional<std::variant<Job, Data>> m_state;
};

// Wrapper which hides some complexity of PackageKit::Transaction management.
// Self-destructs and emiting finished() signal or being cancelled.
class PackageKitFetchDependenciesJob : public QObject
{
    Q_OBJECT

public:
    explicit PackageKitFetchDependenciesJob(const QString &packageId);
    ~PackageKitFetchDependenciesJob() override;

    void cancel();

Q_SIGNALS:
    void finished(QList<PackageKitDependency> dependencies);

private Q_SLOTS:
    void onTransactionErrorCode(PackageKit::Transaction::Error error, const QString &details);
    void onTransactionPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
    void onTransactionFinished();

private:
    QPointer<PackageKit::Transaction> m_transaction;
    QList<PackageKitDependency> m_dependencies;
};
