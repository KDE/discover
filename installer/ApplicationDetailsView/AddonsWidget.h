/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef ADDONSWIDGET_H
#define ADDONSWIDGET_H

#include <KVBox>

#include <LibQApt/Package>

class QListView;
class QModelIndex;
class QPushButton;
class QStandardItemModel;
class QToolButton;

class ApplicationBackend;

class AddonsWidget : public KVBox
{
    Q_OBJECT
public:
    AddonsWidget(QWidget *parent, ApplicationBackend *appBackend);
    ~AddonsWidget();

    void setAddons(QApt::PackageList addons);

private:
    ApplicationBackend *m_appBackend;
    QStandardItemModel *m_addonsModel;
    QApt::PackageList m_availableAddons;
    QHash<QApt::Package *, QApt::Package::State> m_changedAddons;

    QToolButton *m_expandButton;
    QWidget *m_addonsWidget;
    QListView *m_addonsView;
    QPushButton *m_addonsRevertButton;
    QPushButton *m_addonsApplyButton;

public Q_SLOTS:
    void repaintViewport();

private Q_SLOTS:
    void populateModel();
    void expandButtonClicked();
    void addonStateChanged(const QModelIndex &left, const QModelIndex &right);
    void emitApplyButtonClicked();

Q_SIGNALS:
    void applyButtonClicked(QHash<QApt::Package *, QApt::Package::State> addons);
};

#endif
