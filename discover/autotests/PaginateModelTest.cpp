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
#include <QStringListModel>

class PaginateModelTest : public QObject
{
    Q_OBJECT
public:
    PaginateModelTest()
        : m_testModel(new QStringListModel)
    {
        QStringList testValues;
        for(int i=0; i<13; ++i) {
            testValues += QStringLiteral("figui%1").arg(i);
        }
        m_testModel->setStringList(testValues);
    }

private Q_SLOTS:
    void testPages() {
        PaginateModel pm;
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
        QCOMPARE(pm.currentPage(), 0);
        pm.lastPage();
        QCOMPARE(pm.firstItem(), 10);
        QCOMPARE(pm.currentPage(), 2);
    }

private:
    QStringListModel* const m_testModel;
};

QTEST_MAIN( PaginateModelTest )

#include "PaginateModelTest.moc"
