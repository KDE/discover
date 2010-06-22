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

#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include <KTabWidget>

class QLabel;
class QTreeWidget;

class KMenu;
class KPushButton;
class KVBox;
class KTreeWidgetSearchLineWidget;

namespace Ui {
    class MainTab;
}

namespace QApt {
    class Package;
}

class DetailsWidget : public KTabWidget
{
    Q_OBJECT
public:
    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

    void setPackage(QApt::Package *package);

private:
    QApt::Package *m_package;
    Ui::MainTab *m_mainTab;
    KPushButton *m_screenshotButton;


    QWidget *m_technicalTab;


    QWidget *m_dependenciesTab;


    KVBox *m_filesTab;
    KTreeWidgetSearchLineWidget *m_filesSearchEdit;
    QTreeWidget *m_filesTreeWidget;

    QWidget *m_changelogTab;

private Q_SLOTS:
    void setupButtons(QApt::Package *oldPackage);
    void refreshButtons();
    void populateFileList(QApt::Package *package);
};

#endif
