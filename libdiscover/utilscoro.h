/*
 *   SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"

#include <QCoroCore>

#include <chrono>

using namespace std::chrono_literals;

struct DISCOVERCOMMON_EXPORT CoroutineSplitter {
    explicit CoroutineSplitter(std::chrono::milliseconds executionDurationLimit = 10ms, std::chrono::milliseconds pauseDuration = 2ms);
    QCoro::Task<> operator()();

private:
    std::chrono::milliseconds m_executionDurationLimit;
    std::chrono::milliseconds m_pauseDuration;
    std::chrono::time_point<std::chrono::steady_clock> m_lastStart;
};
