// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <resources/AbstractResourcesBackend.h>

class StandardBackendUpdater;
class KDELinuxResource;
class KDELinuxBackend : public AbstractResourcesBackend
{
    Q_OBJECT
public:
    explicit KDELinuxBackend(QObject *parent = nullptr);

    [[nodiscard]] AbstractBackendUpdater *backendUpdater() const override;
    [[nodiscard]] AbstractReviewsBackend *reviewsBackend() const override;

    [[nodiscard]] int updatesCount() const override;
    [[nodiscard]] bool isValid() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] ResultsStream *search(const Filters &search) override;
    [[nodiscard]] bool isFetching() const override;
    [[nodiscard]] Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    [[nodiscard]] Transaction *removeApplication(AbstractResource *app) override;

    void checkForUpdates() override;

private:
    StandardBackendUpdater *m_updater;
    std::shared_ptr<KDELinuxResource> m_resource;
    bool m_fetching = false;
};
