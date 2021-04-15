/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
 
#ifndef RPMOSTREENOTIFIER_H
#define RPMOSTREENOTIFIER_H

#include <BackendNotifierModule.h>
#include <QDebug>

class RpmOstreeNotifier : public BackendNotifierModule
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.discover.BackendNotifierModule")
    Q_INTERFACES(BackendNotifierModule)
public:
    explicit RpmOstreeNotifier(QObject *parent = nullptr);
    ~RpmOstreeNotifier() override;

    void recheckSystemUpdateNeeded() override;
    bool hasSecurityUpdates() override
    {
        return false;
    }
    bool hasUpdates() override;
    bool needsReboot() const override;

private:
    bool m_hasUpdates = false;
    bool m_needsReboot = false;

    /*
     * Getting the output resulting from executing the QProcess update check.
     * and setting m_newUpdate to true if there is a new version.
     */
    void readUpdateOutput(QIODevice *device);

    /*
     * It is executed whenever there is a change in the tracked file (/tmp/discover-ostree-changed)
     * to check if the installation of the new update is finished and
     * the system needs to be rebooted.
     */
    void nowNeedsReboot();
};

#endif
