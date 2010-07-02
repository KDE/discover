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

#ifndef MAINTAB_H
#define MAINTAB_H

#include <QtGui/QWidget>

class QPushButton;

class KAction;
class KJob;
class KMenu;
class KTemporaryFile;

namespace QApt {
    class Package;
}

namespace Ui {
    class MainTab;
}

class MainTab : public QWidget
{
    Q_OBJECT
public:
    explicit MainTab(QWidget *parent = 0);
    ~MainTab();

private:
    Ui::MainTab *m_mainTab;
    QApt::Package *m_package;

    QPushButton *m_screenshotButton;
    KAction *m_purgeAction;
    KMenu *m_purgeMenu;
    KTemporaryFile *m_screenshotFile;

public Q_SLOTS:
    void setPackage(QApt::Package *package);
    void refreshButtons();
    void clear();

private Q_SLOTS:
    void setupButtons(QApt::Package *oldPackage);
    void fetchScreenshot();
    void screenshotFetched(KJob *job);
    void setInstall();
    void setRemove();
    void setUpgrade();
    void setReInstall();
    void setPurge();
    void setKeep();
    bool willCacheBreak();
};

#endif
