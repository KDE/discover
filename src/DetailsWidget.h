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

// KDE inclues
#include <KTabWidget>

class QScrollArea;

namespace QApt {
    class Backend;
    class Package;
}

class MainTab;
class TechnicalDetailsTab;
class ChangelogTab;
class InstalledFilesTab;

class DetailsWidget : public KTabWidget
{
    Q_OBJECT
public:
    explicit DetailsWidget(QWidget *parent);
    ~DetailsWidget();

private:
    QApt::Backend *m_backend;
    QApt::Package *m_package;

    MainTab *m_mainTab;
    TechnicalDetailsTab *m_technicalTab;
    // QWidget *m_dependenciesTab;
    InstalledFilesTab *m_filesTab;
    ChangelogTab *m_changelogTab;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void setPackage(QApt::Package *package);
    void refreshMainTabButtons();
    void clear();
};

#endif
