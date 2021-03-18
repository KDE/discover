/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LOCALFILEPKRESOURCE_H
#define LOCALFILEPKRESOURCE_H

#include "PackageKitResource.h"

class LocalFilePKResource : public PackageKitResource
{
    Q_OBJECT
public:
    LocalFilePKResource(QUrl path, PackageKitBackend *parent);

    QString name() const override;
    QString comment() override;

    AbstractResource::State state() override
    {
        return m_state;
    }
    int size() override;
    void markInstalled();
    QString origin() const override;
    void fetchDetails() override;
    bool canExecute() const override
    {
        return !m_exec.isEmpty();
    }
    void invokeApplication() const override;
    QString displayOrigin() const override
    {
        return origin();
    }

    void setDetails(const PackageKit::Details &details);

private:
    AbstractResource::State m_state = AbstractResource::None;
    QUrl m_path;
    QString m_exec;
};

#endif // LOCALFILEPKRESOURCE_H
