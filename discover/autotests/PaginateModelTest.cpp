/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtTest>
#include "../PaginateModel.h"
#include <tests/modeltest.h>
#include <QStandardItemModel>

class PaginateModelTest : public QObject
{
    Q_OBJECT
public:
    PaginateModelTest()
        : m_testModel(new QStandardItemModel)
    {
        for(int i=0; i<13; ++i) {
            m_testModel->appendRow(new QStandardItem(QStringLiteral("figui%1").arg(i)));
        }
    }

private Q_SLOTS:
    void testPages() {
        PaginateModel pm;
        new ModelTest(&pm, &pm);
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

    void testPageSize() {
        PaginateModel pm;
        new ModelTest(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
        pm.setPageSize(10);
        QCOMPARE(pm.pageCount(), 2);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
    }

    void testItemAdded() {
        PaginateModel pm;
        new ModelTest(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 3);
        QSignalSpy spy(&pm, &QAbstractItemModel::rowsAboutToBeInserted);
        m_testModel->insertRow(3, new QStandardItem(QStringLiteral("mwahahaha")));
        m_testModel->insertRow(3, new QStandardItem(QStringLiteral("mwahahaha")));
        QCOMPARE(spy.count(), 0);
        m_testModel->appendRow(new QStandardItem(QStringLiteral("mwahahaha")));

        pm.lastPage();
        for (int i=0; i<7; ++i)
            m_testModel->appendRow(new QStandardItem(QStringLiteral("mwahahaha%1").arg(i)));
        QCOMPARE(spy.count(), 4);
        pm.firstPage();

        for (int i=0; i<7; ++i)
            m_testModel->appendRow(new QStandardItem(QStringLiteral("faraway%1").arg(i)));
        QCOMPARE(spy.count(), 4);
    }

    void testItemRemoved() {
        PaginateModel pm;
        new ModelTest(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        QCOMPARE(pm.pageCount(), 5);
        QSignalSpy spy(&pm, &QAbstractItemModel::rowsAboutToBeRemoved);
        m_testModel->removeRow(3);
        QCOMPARE(spy.count(), 0);
        spy.clear();

        pm.lastPage();
        m_testModel->removeRow(m_testModel->rowCount()-1);
        QCOMPARE(spy.count(), 1);
    }

    void testMove() {
        PaginateModel pm;
        new ModelTest(&pm, &pm);
        pm.setSourceModel(m_testModel);
        pm.setPageSize(5);
        m_testModel->moveRow({}, 0, {}, 3);
    }

private:
    QStandardItemModel* const m_testModel;
};

QTEST_MAIN( PaginateModelTest )

#include "PaginateModelTest.moc"
