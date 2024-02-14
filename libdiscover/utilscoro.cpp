/*
 *   SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "utilscoro.h"

CoroutineSplitter::CoroutineSplitter(std::chrono::milliseconds executionDurationLimit, std::chrono::milliseconds pauseDuration)
    : m_executionDurationLimit(executionDurationLimit)
    , m_pauseDuration(pauseDuration)
    , m_lastStart(std::chrono::steady_clock::now())
{
}

QCoro::Task<> CoroutineSplitter::operator()()
{
    const auto elapsed = std::chrono::steady_clock::now() - m_lastStart;
    if (elapsed > m_executionDurationLimit) {
        co_await QCoro::sleepFor(m_pauseDuration);
        m_lastStart = std::chrono::steady_clock::now();
    }
}
