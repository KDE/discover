/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "../PaginateModel.h"
#include <QAbstractItemModelTester>
#include <QStringListModel>
#include <QtTest>

void insertRow(QStringListModel *model, int row, const QString &appendString)
{
    model->insertRow(row);
    model->setData(model->index(row, 0), appendString);
}
void appendRow(QStringListModel *model, const QString &appendString)
{
    int count = model->rowCount();
    insertRow(model, count, appendString);
}

class PaginateModelTest : public QObject
{
    Q_OBJECT
public:
    PaginateModelTest()
        : m_testModel(new QStringListModel)
    {
        for (int i = 0; i < 13; ++i) {
            appendRow(m_testModel, QStringLiteral("figui%1").arg(i));
        }
    }

private Q_SLOTS:
    void testPages()
    {
        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
        QCOMPARE(pm.rowCount(), 5);
        QCOMPARE(pm.firstItem(), 0);
        QCOMPARE(pm.currentPage(), 0);
        pm.nextPage();
        QCOMPARE(pm.rowCount(), 5);
        QCOMPARE(pm.currentPage(), 1);
        pm.nextPage();
        QCOMPARE(pm.rowCount(), 3);
        QCOMPARE(pm.currentPage(), 2);

        pm.firstPage();
        QCOMPARE(pm.firstItem(), 0);
        pm.setFirstItem(0);
        QCOMPARE(pm.firstItem(), 0);
        QCOMPARE(pm.currentPage(), 0);
        pm.lastPage();
        QCOMPARE(pm.firstItem(), 10);
        QCOMPARE(pm.currentPage(), 2);
    }

    void testPageSize()
    {
        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
        pm.setPageSize(10);
        QCOMPARE(pm.pageCount(), 2);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
    }

    void testItemAdded()
    {
        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
        QSignalSpy spy(&pm, &QAbstractItemModel::rowsAboutToBeInserted);
        insertRow(m_testModel, 3, QStringLiteral("mwahahaha"));
        insertRow(m_testModel, 3, QStringLiteral("mwahahaha"));
        QCOMPARE(spy.count(), 0);
        appendRow(m_testModel, QStringLiteral("mwahahaha"));

        pm.lastPage();
        for (int i = 0; i < 7; ++i)
            appendRow(m_testModel, QStringLiteral("mwahahaha%1").arg(i));
        QCOMPARE(spy.count(), 4);
        pm.firstPage();

        for (int i = 0; i < 7; ++i)
            appendRow(m_testModel, QStringLiteral("faraway%1").arg(i));
        QCOMPARE(spy.count(), 4);
    }

    void testItemAddBeginning()
    {
        QStringListModel smallerModel;

        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(&smallerModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 1);
        QCOMPARE(pm.rowCount(), 0);
        insertRow(&smallerModel, 0, QStringLiteral("just one"));
        QCOMPARE(pm.pageCount(), 1);
        QCOMPARE(pm.rowCount(), 1);
        smallerModel.removeRow(0);
        QCOMPARE(pm.pageCount(), 1);
        QCOMPARE(pm.rowCount(), 0);
    }

    void testItemRemoved()
    {
        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 5);
        QSignalSpy spy(&pm, &QAbstractItemModel::rowsAboutToBeRemoved);
        m_testModel->removeRow(3);
        QCOMPARE(spy.count(), 0);
        spy.clear();

        pm.lastPage();
        m_testModel->removeRow(m_testModel->rowCount() - 1);
        QCOMPARE(spy.count(), 1);
    }

    void testMove()
    {
        PaginateModel pm;
        new QAbstractItemModelTester(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        m_testModel->moveRow({}, 0, {}, 3);
    }

private:
    QStringListModel *const m_testModel;
};

QTEST_MAIN(PaginateModelTest)

#include "PaginateModelTest.moc"
