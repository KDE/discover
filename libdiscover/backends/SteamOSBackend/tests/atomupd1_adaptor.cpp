/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Source file was com.steampowered.Atomupd1.xml
 *
 * qdbusxml2cpp is Copyright (C) The Qt Company Ltd. and other contributors.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "atomupd1_adaptor.h"
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "mock_server.h"

/*
 * Implementation of adaptor class Atomupd1Adaptor
 */

Atomupd1Adaptor::Atomupd1Adaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<VariantMapMap>();
    // constructor
    setAutoRelaySignals(true);
}

Atomupd1Adaptor::~Atomupd1Adaptor()
{
    // destructor
}

QString Atomupd1Adaptor::branch() const
{
    // get the value of property Branch
    return qvariant_cast<QString>(parent()->property("Branch"));
}

QString Atomupd1Adaptor::currentBuildID() const
{
    // get the value of property CurrentBuildID
    return qvariant_cast<QString>(parent()->property("CurrentBuildID"));
}

QString Atomupd1Adaptor::currentVersion() const
{
    // get the value of property CurrentVersion
    return qvariant_cast<QString>(parent()->property("CurrentVersion"));
}

qulonglong Atomupd1Adaptor::estimatedCompletionTime() const
{
    // get the value of property EstimatedCompletionTime
    return qvariant_cast<qulonglong>(parent()->property("EstimatedCompletionTime"));
}

QString Atomupd1Adaptor::failureCode() const
{
    // get the value of property FailureCode
    return qvariant_cast<QString>(parent()->property("FailureCode"));
}

QString Atomupd1Adaptor::failureMessage() const
{
    // get the value of property FailureMessage
    return qvariant_cast<QString>(parent()->property("FailureMessage"));
}

HTTPProxy Atomupd1Adaptor::httpProxy() const
{
    // get the value of property HttpProxy
    return qvariant_cast<HTTPProxy>(parent()->property("HttpProxy"));
}

QStringList Atomupd1Adaptor::knownBranches() const
{
    // get the value of property KnownBranches
    return qvariant_cast<QStringList>(parent()->property("KnownBranches"));
}

QStringList Atomupd1Adaptor::knownVariants() const
{
    // get the value of property KnownVariants
    return qvariant_cast<QStringList>(parent()->property("KnownVariants"));
}

double Atomupd1Adaptor::progressPercentage() const
{
    // get the value of property ProgressPercentage
    return qvariant_cast<double>(parent()->property("ProgressPercentage"));
}

QString Atomupd1Adaptor::updateBuildID() const
{
    // get the value of property UpdateBuildID
    return qvariant_cast<QString>(parent()->property("UpdateBuildID"));
}

uint Atomupd1Adaptor::updateStatus() const
{
    // get the value of property UpdateStatus
    return qvariant_cast<uint>(parent()->property("UpdateStatus"));
}

QString Atomupd1Adaptor::updateVersion() const
{
    // get the value of property UpdateVersion
    return qvariant_cast<QString>(parent()->property("UpdateVersion"));
}

VariantMapMap Atomupd1Adaptor::updatesAvailable() const
{
    // get the value of property UpdatesAvailable
    return qvariant_cast<VariantMapMap>(parent()->property("UpdatesAvailable"));
}

VariantMapMap Atomupd1Adaptor::updatesAvailableLater() const
{
    // get the value of property UpdatesAvailableLater
    return qvariant_cast<VariantMapMap>(parent()->property("UpdatesAvailableLater"));
}

QString Atomupd1Adaptor::variant() const
{
    // get the value of property Variant
    return qvariant_cast<QString>(parent()->property("Variant"));
}

uint Atomupd1Adaptor::version() const
{
    // get the value of property Version
    return qvariant_cast<uint>(parent()->property("Version"));
}

void Atomupd1Adaptor::CancelUpdate()
{
    // handle method call com.steampowered.Atomupd1.CancelUpdate
    QMetaObject::invokeMethod(parent(), "CancelUpdate");
}

VariantMapMap Atomupd1Adaptor::CheckForUpdates(const QVariantMap &options, VariantMapMap &updates_available_later)
{
    // handle method call com.steampowered.Atomupd1.CheckForUpdates
    return static_cast<MockServer *>(parent())->CheckForUpdates(options, updates_available_later);
}

void Atomupd1Adaptor::DisableHttpProxy()
{
    // handle method call com.steampowered.Atomupd1.DisableHttpProxy
    QMetaObject::invokeMethod(parent(), "DisableHttpProxy");
}

void Atomupd1Adaptor::EnableHttpProxy(const QString &address, int port, const QVariantMap &options)
{
    // handle method call com.steampowered.Atomupd1.EnableHttpProxy
    QMetaObject::invokeMethod(parent(), "EnableHttpProxy", Q_ARG(QString, address), Q_ARG(int, port), Q_ARG(QVariantMap, options));
}

void Atomupd1Adaptor::PauseUpdate()
{
    // handle method call com.steampowered.Atomupd1.PauseUpdate
    QMetaObject::invokeMethod(parent(), "PauseUpdate");
}

void Atomupd1Adaptor::ReloadConfiguration(const QVariantMap &options)
{
    // handle method call com.steampowered.Atomupd1.ReloadConfiguration
    QMetaObject::invokeMethod(parent(), "ReloadConfiguration", Q_ARG(QVariantMap, options));
}

void Atomupd1Adaptor::ResumeUpdate()
{
    // handle method call com.steampowered.Atomupd1.ResumeUpdate
    QMetaObject::invokeMethod(parent(), "ResumeUpdate");
}

void Atomupd1Adaptor::StartUpdate(const QString &id)
{
    // handle method call com.steampowered.Atomupd1.StartUpdate
    QMetaObject::invokeMethod(parent(), "StartUpdate", Q_ARG(QString, id));
}

void Atomupd1Adaptor::SwitchToBranch(const QString &branch)
{
    // handle method call com.steampowered.Atomupd1.SwitchToBranch
    QMetaObject::invokeMethod(parent(), "SwitchToBranch", Q_ARG(QString, branch));
}

void Atomupd1Adaptor::SwitchToVariant(const QString &variant)
{
    // handle method call com.steampowered.Atomupd1.SwitchToVariant
    QMetaObject::invokeMethod(parent(), "SwitchToVariant", Q_ARG(QString, variant));
}
