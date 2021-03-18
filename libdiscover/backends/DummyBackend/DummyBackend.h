/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DUMMYBACKEND_H
#define DUMMYBACKEND_H

#include <QVariantList>
#include <resources/AbstractResourcesBackend.h>

class DummyReviewsBackend;
class StandardBackendUpdater;
class DummyResource;
class DummyBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_PROPERTY(int startElements MEMBER m_startElements)
public:
    explicit DummyBackend(QObject *parent = nullptr);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    ResultsStream *findResourceByPackageName(const QUrl &search);
    QHash<QString, DummyResource *> resources() const
    {
        return m_resources;
    }
    bool isValid() const override
    {
        return true;
    } // No external file dependencies that could cause runtime errors

    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;
    bool isFetching() const override
    {
        return m_fetching;
    }
    void checkForUpdates() override;
    QString displayName() const override;
    bool hasApplications() const override;

public Q_SLOTS:
    void toggleFetching();

private:
    void populate(const QString &name);

    QHash<QString, DummyResource *> m_resources;
    StandardBackendUpdater *m_updater;
    DummyReviewsBackend *m_reviews;
    bool m_fetching;
    int m_startElements;
};

#endif // DUMMYBACKEND_H
