/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef PACKAGEWIDGET_H
#define PACKAGEWIDGET_H

// Qt includes
#include <QModelIndex>
#include <QLabel>
#include <QSplitter>

class QVBoxLayout;

class KLineEdit;

class DetailsWidget;
class PackageModel;
class PackageProxyModel;
class PackageView;

namespace QApt
{
    class Backend;
}

class PackageWidget : public QSplitter
{
    Q_OBJECT
public:
    enum PackagesType {
        AvailablePackages = 1,
        UpgradeablePackages = 2,
        MarkedPackages = 3
    };

    PackageWidget(QWidget *parent);
    ~PackageWidget();

    void setHeaderWidget(QWidget *widget);
    QWidget *headerWidget() {
        return m_headerWidget;
    }

    void setPackagesType(int type);
    int packagesType() {
        return m_packagesType;
    }

protected:
    QApt::Backend *m_backend;
    PackageView *m_packageView;
    DetailsWidget *m_detailsWidget;
    PackageModel *m_model;
    PackageProxyModel *m_proxyModel;

private:
    QWidget *m_headerWidget;
    QVBoxLayout* m_topLayout;

    int m_packagesType;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void setPackages();

private Q_SLOTS:
    void packageActivated(const QModelIndex &index);
};

#endif
