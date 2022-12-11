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
    case Format::Classic: {
        // Get remote and ref from the ostree source
        auto split_ref = source.split(':');
        if (split_ref.length() != 2) {
            // Unknown or invalid format
            return;
        }
        m_remote = split_ref[0];
        m_branch = split_ref[1];
        // Set the format now that we have a valid remote and ref
        m_format = OstreeFormat::Classic;
        break;
    }

    // Using new OCI container format
    case Format::OCI: {
        auto split_ref = source.split(':');
        if ((split_ref.length() < 2) || (split_ref.length() > 3)) {
            // Unknown or invalid format
            return;
        }
        if (split_ref[0] != QStringLiteral("ostree-unverified-registry")) {
            // Unknown or invalid format
            return;
        }
        m_remote = split_ref[1];
        if (split_ref.length() == 3) {
            m_branch = split_ref[2];
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
