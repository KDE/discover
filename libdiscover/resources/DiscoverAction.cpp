/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverAction.h"

DiscoverAction::DiscoverAction(QObject* parent)
    : QObject(parent)
{
}

DiscoverAction::DiscoverAction(const QIcon icon, const QString &text, QObject* parent)
    : QObject(parent)
    , m_text(text)
    , m_icon(icon)
{
}

DiscoverAction::DiscoverAction(const QString &text, QObject* parent)
    : QObject(parent)
    , m_text(text)
{
}

void DiscoverAction::setEnabled(bool enabled)
{
    if (enabled == m_isEnabled)
        return;

    m_isEnabled = enabled;
    Q_EMIT enabledChanged(enabled);
}

void DiscoverAction::setVisible(bool visible)
{
    if (visible == m_isVisible)
        return;

    m_isVisible = visible;
    Q_EMIT visibleChanged(visible);
}

void DiscoverAction::setIcon(const QIcon& icon)
{
    if (icon.name() == m_icon.name() && !icon.name().isEmpty())
        return;

    m_icon = icon;
    Q_EMIT iconChanged(icon);
}

void DiscoverAction::setText(const QString& text)
{
    if (text == m_text)
        return;

    m_text = text;
    Q_EMIT textChanged(text);
}

void DiscoverAction::setToolTip(const QString& toolTip)
{
    if (toolTip == m_toolTip)
        return;

    m_toolTip = toolTip;
    Q_EMIT toolTipChanged(toolTip);
}

void DiscoverAction::trigger()
{
    Q_EMIT triggered();
}
