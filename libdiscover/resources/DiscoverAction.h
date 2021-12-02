/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"
#include <QObject>

/**
 * An action class that doesn't need QtWidgets
 */
class DISCOVERCOMMON_EXPORT DiscoverAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
public:
    DiscoverAction(QObject *parent = nullptr);
    DiscoverAction(const QString &text, QObject *parent = nullptr);
    DiscoverAction(const QString &icon, const QString &text, QObject *parent = nullptr);

    void setText(const QString &text);
    void setToolTip(const QString &toolTip);
    void setIconName(const QString &iconName);
    void setEnabled(bool enabled);
    void setVisible(bool enabled);

    bool isVisible() const
    {
        return m_isVisible;
    }

    bool isEnabled() const
    {
        return m_isEnabled;
    }

    QString text() const
    {
        return m_text;
    }

    QString toolTip() const
    {
        return m_toolTip;
    }

    QString iconName() const
    {
        return m_icon;
    }

public Q_SLOTS:
    void trigger();

Q_SIGNALS:
    void triggered();

    void textChanged(const QString &text);
    void toolTipChanged(const QString &toolTip);
    void iconNameChanged(const QString &iconName);
    void visibleChanged(bool isVisible);
    void enabledChanged(bool isEnabled);

private:
    bool m_isVisible = true;
    bool m_isEnabled = true;
    QString m_text;
    QString m_toolTip;
    QString m_icon;
};
