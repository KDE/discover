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

#ifndef TECHNICALDETAILSTAB_H
#define TECHNICALDETAILSTAB_H

#include <QtGui/QScrollArea>

class QGroupBox;
class QLabel;

namespace QApt {
    class Package;
}

class MuonStrings;

class TechnicalDetailsTab : public QScrollArea
{
    Q_OBJECT
public:
    explicit TechnicalDetailsTab(QWidget *parent = 0);
    ~TechnicalDetailsTab();

private:
    QApt::Package *m_package;
    MuonStrings *m_strings;

    QLabel *m_maintainer;
    QLabel *m_section;
    QLabel *m_sourcePackage;

    QGroupBox *m_installedVersionBox;
    QGroupBox *m_currentVersionBox;

    QLabel *m_installedVersion;
    QLabel *m_installedSize;
    QLabel *m_currentVersion;
    QLabel *m_currentSize;
    QLabel *m_downloadSize;

public Q_SLOTS:
    void setPackage(QApt::Package *package);
    void refresh();
};

#endif
