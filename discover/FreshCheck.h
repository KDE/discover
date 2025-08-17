/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QThread>

class FreshThread : public QThread
{
    Q_OBJECT
public:
    FreshThread(int interval, QAtomicInteger<qint64> &lastTrigger)
        : m_interval(interval)
        , m_lastTrigger(lastTrigger)
    {
    }

    void run() override;

    void check();

private:
    const int m_interval;
    QAtomicInteger<qint64> &m_lastTrigger;
};

class Q_DECL_EXPORT FreshCheck : public QObject
{
public:
    FreshCheck(int interval = 20, int margin = 40, QObject *parent = nullptr);

private:
    void check();

    const int m_margin;
    QElapsedTimer m_elapsed;
    //     FreshThread* const m_thread;
    QAtomicInteger<qint64> m_lastTrigger;
};
