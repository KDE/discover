// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

void deleteLaterUniqueDeleter(QObject *object);

struct DeleteLaterDeleter {
    void operator()(QObject *p) const
    {
        p->deleteLater();
    }
};

template<typename T, typename... Args>
inline auto makeDeleteLaterShared(Args &&...args)
{
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...), DeleteLaterDeleter());
}
