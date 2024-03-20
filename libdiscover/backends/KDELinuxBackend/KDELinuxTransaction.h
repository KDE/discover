// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <Transaction/Transaction.h>

class KDELinuxTransaction : public Transaction
{
    Q_OBJECT
public:
    KDELinuxTransaction(QObject *parent, const std::shared_ptr<AbstractResource> &resource);
    void cancel() override;

private:
    std::shared_ptr<AbstractResource> m_resource;
};
