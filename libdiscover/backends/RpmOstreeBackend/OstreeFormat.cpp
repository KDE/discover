/*
 *   SPDX-FileCopyrightText: 2022 Timothée Ravier <tim@siosm.fr>
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "OstreeFormat.h"

OstreeFormat::OstreeFormat(Format format, const QString &source)
    : m_format(Format::Unknown)
{
    if (source.isEmpty()) {
        return;
    }

    switch (format) {
    // Using classic ostree image format
    // Examples:
    // fedora:fedora/38/x86_64/kinoite
    // fedora/38/x86_64/kinoite
    case Format::Classic: {
        // Get remote and ref from the ostree source
        auto split_ref = source.split(QLatin1Char(':'));
        if (split_ref.length() != 2) {
            // Unknown or invalid format
            m_remote = QString();
            m_branch = source;
            return;
        }
        m_remote = split_ref[0];
        m_branch = split_ref[1];
        // Set the format now that we have a valid remote and ref
        m_format = OstreeFormat::Classic;
        break;
    }

    // Using new OCI container format
    // Examples:
    // ostree-unverified-image:registry:<oci image>
    // ostree-unverified-image:docker://<oci image>
    // ostree-unverified-registry:<oci image>
    // ostree-remote-image:<ostree remote>:registry:<oci image>
    // ostree-remote-image:<ostree remote>:docker://<oci image>
    // ostree-remote-registry:<ostree remote>:<oci image>
    // ostree-image-signed:registry:<oci image>
    // ostree-image-signed:docker://<oci image>
    case Format::OCI: {
        // All early returns correspond to an unknown or invalid format
        QStringList split = source.split(QLatin1Char(':'));
        if ((split.length() < 2) || (split.length() > 5)) {
            return;
        }

        // First figure out the ostree url format and the transport
        if (split.first() == QLatin1String("ostree-image-signed")) {
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
            if (!parseTransport(&split)) {
                return;
            }
        } else if (split.first() == QLatin1String("ostree-unverified-image")) {
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
            if (!parseTransport(&split)) {
                return;
            }
        } else if (split.first() == QLatin1String("ostree-unverified-registry")) {
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
        } else if (split.first() == QLatin1String("ostree-remote-image")) {
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
            // Append the ostree remote name
            if (split.empty()) {
                return;
            }
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
            if (!parseTransport(&split)) {
                return;
            }
        } else if (split.first() == QLatin1String("ostree-remote-registry")) {
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
            // Append the ostree remote name
            if (split.empty()) {
                return;
            }
            m_transport = split.first() + QLatin1Char(':');
            split.removeFirst();
        } else {
            return;
        }

        // Now figure out the repo, image name and optional tag
        if (split.empty()) {
            return;
        }
        m_remote = split.first();
        split.removeFirst();
        if (!split.isEmpty()) {
            m_branch = split.first();
        } else {
            m_branch = QStringLiteral("latest");
        }

        // Set the format now that we have a valid repo and tag
        m_format = OstreeFormat::OCI;
        break;
    }

    // This should never happen and should fail at compilation time as we're using an enum
    default:
        Q_ASSERT(false);
    }
}

// All transports listed in https://github.com/containers/image/blob/main/docs/containers-transports.5.md
bool OstreeFormat::parseTransport(QStringList *split)
{
    if (split->empty()) {
        return false;
    }
    // Special case for the 'docker://' transport
    if (split->first() == QLatin1String("docker")) {
        m_transport += QLatin1String("docker://");
        split->removeFirst();
        if (split->empty()) {
            return false;
        }
        if (!split->first().startsWith(QLatin1String("//"))) {
            return false;
        }
        split->first() = split->first().remove(0, 2);
    } else if (split->first() == QLatin1String("registry") || split->first() == QLatin1String("oci") || split->first() == QLatin1String("oci-archive")
               || split->first() == QLatin1String("containers-storage") || split->first() == QLatin1String("dir")) {
        m_transport = split->first() + QLatin1Char(':');
        split->removeFirst();
    } else {
        // No known/valid transport found
        return false;
    }
    return true;
}

bool OstreeFormat::isValid() const
{
    return m_format == Format::Classic || m_format == Format::OCI;
}

bool OstreeFormat::isClassic() const
{
    return m_format == Format::Classic;
}

bool OstreeFormat::isOCI() const
{
    return m_format == Format::OCI;
}

QString OstreeFormat::remote() const
{
    if (m_format != Format::Classic) {
        return QStringLiteral("unknown");
    }
    return m_remote;
}

QString OstreeFormat::ref() const
{
    if (m_format != Format::Classic) {
        return QStringLiteral("unknown");
    }
    return m_branch;
}

QString OstreeFormat::repo() const
{
    if (m_format != Format::OCI) {
        return QStringLiteral("unknown");
    }
    return m_remote;
}

QString OstreeFormat::tag() const
{
    if (m_format != Format::OCI) {
        return QStringLiteral("unknown");
    }
    return m_branch;
}

QString OstreeFormat::transport() const
{
    if (m_format != Format::OCI) {
        return QStringLiteral("unknown");
    }
    return m_transport;
}

#include "moc_OstreeFormat.cpp"
