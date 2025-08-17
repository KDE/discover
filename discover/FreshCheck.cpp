/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "FreshCheck.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QTimer>

FreshCheck::FreshCheck(int interval, int margin, QObject *parent)
    : QObject(parent)
    , m_margin(margin)
    , m_lastTrigger(QDateTime::currentMSecsSinceEpoch())
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(interval);
    timer->setSingleShot(false);
    timer->start();
    m_elapsed.start();
    connect(timer, &QTimer::timeout, this, &FreshCheck::check);

    auto thread = new FreshThread(interval, m_lastTrigger);
    thread->start(QThread::HighPriority);
    connect(this, &QObject::destroyed, thread, &FreshThread::quit);
}

void FreshCheck::check()
{
    if (m_elapsed.elapsed() > m_margin) {
        qDebug() << "The" << m_margin << "timer took" << m_elapsed.elapsed();
    } else {
        // qDebug() << "good mec" << m_elapsed.elapsed();
    }
    m_elapsed.restart();
    m_lastTrigger = QDateTime::currentMSecsSinceEpoch();
}

void FreshThread::run()
{
    QObject o;
    QTimer *timer = new QTimer(&o);
    timer->setInterval(m_interval);
    timer->setSingleShot(false);
    timer->moveToThread(this);
    timer->start();
    connect(timer, &QTimer::timeout, &o, [this] {
        check();
    });

    qDebug() << "thread started";
    m_lastTrigger = QDateTime::currentMSecsSinceEpoch();
    Q_ASSERT(QThread::currentThread() == this);
    qDebug() << "thread executed" << exec();
}

void FreshThread::check()
{
    Q_ASSERT(QThread::currentThread() == this);
    Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());

    const auto sinceTriggered = QDateTime::currentMSecsSinceEpoch() - m_lastTrigger;
    if (sinceTriggered > m_interval * 20) {
        qDebug() << "Took" << sinceTriggered << "ms to reply.";
        abort();
    }
}
