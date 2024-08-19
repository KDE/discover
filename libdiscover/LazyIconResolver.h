// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include "discovercommon_export.h"

#include <QObject>
#include <QPointer>
#include <QQueue>

#include "resources/AbstractResource.h"

// Lazy icon resolver using the event loop. Since QIcon doesn't make any thread
// safety claims we'll want to resolve icons on the main thread, but in a way
// that doesn't block the UI. The resolver takes care of this by resolving icons
// with a short delay as well as low event priority.
class DISCOVERCOMMON_EXPORT LazyIconResolver : public QObject
{
    Q_OBJECT
public:
    static LazyIconResolver *instance();
    void queue(AbstractResource *resource);

private:
    using QObject::QObject;
    void resolve();
    void customEvent(QEvent *event) override;

    QQueue<QPointer<AbstractResource>> m_resources;
};
