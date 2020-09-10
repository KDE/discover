/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CACHEDNETWORKACCESSMANAGER_H
#define CACHEDNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
#include <KIO/AccessManager>

class Q_DECL_EXPORT CachedNetworkAccessManager : public KIO::AccessManager
{
    Q_OBJECT
public:
    explicit CachedNetworkAccessManager(const QString &path, QObject *parent = nullptr);

    virtual QNetworkReply * createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = nullptr) override;
};

#endif // CACHEDNETWORKACCESSMANAGER_H

