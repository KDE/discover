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

#include "DetailsTab.h"

// Qt includes
#include <QtCore/QVariantMap>

// QApt includes
#include <QApt/Globals>

class QPushButton;
class QLabel;
class QToolButton;

class KMenu;
class KTextBrowser;

class MainTab : public DetailsTab
{
    Q_OBJECT
public:
    explicit MainTab(QWidget *parent);

private:
    QLabel *m_packageShortDescLabel;

    QLabel *m_buttonLabel;
    QPushButton *m_installButton;
    QToolButton *m_removeButton;
    QPushButton *m_upgradeButton;
    QPushButton *m_reinstallButton;
    QAction *m_purgeAction;
    KMenu *m_purgeMenu;
    QPushButton *m_purgeButton;
    QPushButton *m_cancelButton;

    KTextBrowser *m_descriptionBrowser;

public Q_SLOTS:
    void refresh();

private Q_SLOTS:
    void emitSetInstall();
    void emitSetRemove();
    void emitSetUpgrade();
    void emitSetReInstall();
    void emitSetPurge();
    void emitSetKeep();
    void hideButtons();

Q_SIGNALS:
    void setInstall(QApt::Package *package);
    void setRemove(QApt::Package *package);
    void setUpgrade(QApt::Package *package);
    void setReInstall(QApt::Package *package);
    void setKeep(QApt::Package *package);
    void setPurge(QApt::Package *package);
};

#endif
