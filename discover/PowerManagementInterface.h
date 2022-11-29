/*
   SPDX-FileCopyrightText: 2019 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>

   SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef POWERMANAGEMENTINTERFACE_H
#define POWERMANAGEMENTINTERFACE_H

#include <QObject>

#include <memory>

class QDBusPendingCallWatcher;
class PowerManagementInterfacePrivate;

class PowerManagementInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString reason READ reason WRITE setReason)

    Q_PROPERTY(bool preventSleep READ preventSleep WRITE setPreventSleep NOTIFY preventSleepChanged)

    Q_PROPERTY(bool sleepInhibited READ sleepInhibited NOTIFY sleepInhibitedChanged)

public:
    explicit PowerManagementInterface(QObject *parent = nullptr);

    ~PowerManagementInterface() override;

    [[nodiscard]] bool preventSleep() const;

    [[nodiscard]] bool sleepInhibited() const;

    QString reason() const;
    void setReason(const QString &reason);

Q_SIGNALS:

    void preventSleepChanged();

    void sleepInhibitedChanged();

public Q_SLOTS:

    void setPreventSleep(bool value);

    void retryInhibitingSleep();

private Q_SLOTS:

    void hostSleepInhibitChanged();

    void inhibitDBusCallFinished(QDBusPendingCallWatcher *aWatcher);

    void uninhibitDBusCallFinished(QDBusPendingCallWatcher *aWatcher);

private:
    void inhibitSleep();

    void uninhibitSleep();

    std::unique_ptr<PowerManagementInterfacePrivate> d;
};

#endif // POWERMANAGEMENTINTERFACE_H
