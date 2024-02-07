/*
   SPDX-FileCopyrightText: 2019 (c) Matthieu Gallien <matthieu_gallien@yahoo.fr>

   SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "PowerManagementInterface.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>

#include <QString>

#include <QGuiApplication>

#include <QCoro/QCoroDBus>

#include "discover_debug.h"
#include <QLoggingCategory>

class PowerManagementInterfacePrivate
{
public:
    bool mPreventSleep = false;

    bool mInhibitedSleep = false;

    uint mInhibitSleepCookie = 0;

    QString m_reason;
};

PowerManagementInterface::PowerManagementInterface(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<PowerManagementInterfacePrivate>())
{
    auto sessionBus = QDBusConnection::sessionBus();

    sessionBus.connect(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                       QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                       QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                       QStringLiteral("HasInhibitChanged"),
                       this,
                       SLOT(hostSleepInhibitChanged()));
}

PowerManagementInterface::~PowerManagementInterface() = default;

bool PowerManagementInterface::preventSleep() const
{
    return d->mPreventSleep;
}

bool PowerManagementInterface::sleepInhibited() const
{
    return d->mInhibitedSleep;
}

void PowerManagementInterface::setPreventSleep(bool value)
{
    if (d->mPreventSleep == value) {
        return;
    }

    if (value) {
        inhibitSleep();
        d->mPreventSleep = true;
    } else {
        uninhibitSleep();
        d->mPreventSleep = false;
    }

    Q_EMIT preventSleepChanged();
}

void PowerManagementInterface::retryInhibitingSleep()
{
    if (d->mPreventSleep && !d->mInhibitedSleep) {
        inhibitSleep();
    }
}

void PowerManagementInterface::hostSleepInhibitChanged()
{
}

QCoro::Task<> PowerManagementInterface::inhibitSleep()
{
    QPointer<PowerManagementInterface> guard = this;

    auto sessionBus = QDBusConnection::sessionBus();

    auto inhibitCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                      QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("Inhibit"));

    inhibitCall.setArguments({{QGuiApplication::desktopFileName()}, {d->m_reason}});

    QDBusPendingReply<uint> reply = co_await sessionBus.asyncCall(inhibitCall);

    if (!guard) {
        co_return;
    }

    if (reply.isError()) {
        qCWarning(DISCOVER_LOG) << "PowerManagementInterface::inhibitSleep" << reply.error();
    } else {
        d->mInhibitSleepCookie = reply.argumentAt<0>();
        d->mInhibitedSleep = true;

        Q_EMIT sleepInhibitedChanged();
    }
}

QCoro::Task<> PowerManagementInterface::uninhibitSleep()
{
    QPointer<PowerManagementInterface> guard = this;

    auto sessionBus = QDBusConnection::sessionBus();

    auto uninhibitCall = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                        QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                        QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                        QStringLiteral("UnInhibit"));

    uninhibitCall.setArguments({{d->mInhibitSleepCookie}});

    QDBusPendingReply<> reply = co_await sessionBus.asyncCall(uninhibitCall);

    if (!guard) {
        co_return;
    }

    if (reply.isError()) {
        qCWarning(DISCOVER_LOG) << "PowerManagementInterface::uninhibitDBusCallFinished" << reply.error();
    } else {
        d->mInhibitedSleep = false;

        Q_EMIT sleepInhibitedChanged();
    }
}

QString PowerManagementInterface::reason() const
{
    return d->m_reason;
}

void PowerManagementInterface::setReason(const QString &reason)
{
    d->m_reason = reason;
}

#include "moc_PowerManagementInterface.cpp"
