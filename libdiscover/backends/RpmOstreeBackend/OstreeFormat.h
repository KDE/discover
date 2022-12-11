/*
 *   SPDX-FileCopyrightText: 2022 Timothée Ravier <tim@siosm.fr>
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef OSTREEFORMAT_H
#define OSTREEFORMAT_H

#include <QObject>
#include <QString>

/*
 * Represents the different formats used by ostree to deliver images and updates.
 *
 * For the classic ostree format, see:
 * - https://ostreedev.github.io/ostree/repo/#refs
 *
 * For the new container format, see:
 * - https://github.com/ostreedev/ostree-rs-ext
 * - https://coreos.github.io/rpm-ostree/container/
 */
class OstreeFormat : public QObject
{
    Q_GADGET
public:
    /* The format used by ostree to deliver updates */
    enum Format {
        // Classic ostree format with a remote (repo), branch and ref logic similar to Git
        Classic = 0,
        // Container based format (ostree commit encapsulated into a container), with a repo
        // and tag logic
        OCI,
        // Unknown format, used for errors
        Unknown,
    };
    Q_ENUM(Format)

    OstreeFormat(Format format, const QString &source);

    bool isValid() const;
    bool isClassic() const;
    bool isOCI() const;

    QString repo() const;
    QString tag() const;

    QString remote() const;
    QString ref() const;

private:
    /* Store the format used by ostree to pull each deployment */
    Format m_format;

    /* For the classic format: the ostree remote where the image come from
     * For the OCI format: The container repo URL */
    QString m_remote;

    /* For the classic format: the ostree ref (branch/name/version) used for the image
     * For the OCI format: The container image tag */
    QString m_branch;
};

#endif
