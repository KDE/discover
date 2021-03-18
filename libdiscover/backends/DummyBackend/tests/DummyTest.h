/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DUMMYTEST_H
#define DUMMYTEST_H

#include <QObject>

class ResourcesModel;
class AbstractResourcesBackend;

class DummyTest : public QObject
{
    Q_OBJECT
public:
    explicit DummyTest(QObject *parent = nullptr);

private Q_SLOTS:
    void initTestCase();

    void testReadData();
    void testProxy();
    void testProxySorting();
    void testFetch();
    void testSort();
    void testInstallAddons();
    void testReviewsModel();
    void testUpdateModel();
    void testScreenshotsModel();

private:
    AbstractResourcesBackend *m_appBackend;
    ResourcesModel *m_model;
};

#endif // DUMMYTEST_H
