/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QQmlEngine>

#include "discoversettings.h"

#ifdef WITH_FEEDBACK
#include "plasmauserfeedback.h"
#endif

struct DiscoverSettingsForeign {
    Q_GADGET
    QML_FOREIGN(DiscoverSettings)
    QML_SINGLETON
    QML_NAMED_ELEMENT(DiscoverSettings)
public:
    static DiscoverSettings *create(QQmlEngine *engine, QJSEngine *)
    {
        auto r = new DiscoverSettings;
        r->setParent(engine);
        QObject::connect(r, &DiscoverSettings::installedPageSortingChanged, r, &DiscoverSettings::save);
        QObject::connect(r, &DiscoverSettings::appsListPageSortingChanged, r, &DiscoverSettings::save);
        return r;
    }
};

#ifdef WITH_FEEDBACK
struct PlasmaUserFeedbackForeign {
    Q_GADGET
    QML_FOREIGN(PlasmaUserFeedback)
    QML_SINGLETON
    QML_NAMED_ELEMENT(UserFeedbackSettings)
public:
    static PlasmaUserFeedback *create(QQmlEngine *, QJSEngine *)
    {
        return new PlasmaUserFeedback(KSharedConfig::openConfig(QStringLiteral("PlasmaUserFeedback"), KConfig::NoGlobals));
    }
};
#endif
