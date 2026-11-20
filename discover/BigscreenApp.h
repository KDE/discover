// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class QApplication;
class KDBusService;

int runBigscreen(QApplication &app, KDBusService *service);
