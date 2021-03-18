/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BACKENDNOTIFIERMODULE_H
#define BACKENDNOTIFIERMODULE_H

#include "discovernotifiers_export.h"
#include <QObject>

class DISCOVERNOTIFIERS_EXPORT UpgradeAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
public:
    UpgradeAction(const QString &name, const QString &description, QObject *parent)
        : QObject(parent)
        , m_name(name)
        , m_description(description)
    {
    }

    QString name() const
    {
        return m_name;
    }

    QString description() const
    {
        return m_description;
    }

    void trigger()
    {
        Q_EMIT triggered(m_name);
    }

Q_SIGNALS:
    void triggered(const QString &name);

private:
    const QString m_name;
    const QString m_description;
};

class DISCOVERNOTIFIERS_EXPORT BackendNotifierModule : public QObject
{
    Q_OBJECT
public:
    explicit BackendNotifierModule(QObject *parent = nullptr);
    ~BackendNotifierModule() override;

    /*** Check for new updates. Emits @see foundUpdates when it finds something. **/
    virtual void recheckSystemUpdateNeeded() = 0;

    /*** @returns count of !security updates only. **/
    virtual bool hasUpdates() = 0;

    /*** @returns count of security updates only. **/
    virtual bool hasSecurityUpdates() = 0;

    /** @returns whether the system changed in a way that needs to be rebooted. */
    virtual bool needsReboot() const = 0;

Q_SIGNALS:
    /**
     * This signal is emitted when any new updates are available.
     * @see recheckSystemUpdateNeeded
     */
    void foundUpdates();

    /** Notifies that the system needs a reboot. @see needsReboot */
    void needsRebootChanged();

    /** notifies about an available upgrade */
    void foundUpgradeAction(UpgradeAction *action);
};

Q_DECLARE_INTERFACE(BackendNotifierModule, "org.kde.discover.BackendNotifierModule")

#endif
