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

#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

// Qt includes
#include <QtWidgets/QWidget>

class QLabel;
class QProgressBar;
class QTimer;

namespace QApt {
    class Backend;
}

class StatusWidget : public QWidget
{
    Q_OBJECT
public:
    StatusWidget(QWidget *parent);

private:
    QApt::Backend *m_backend;

    QLabel *m_countsLabel;
    QLabel *m_changesLabel;
    QLabel *m_downloadLabel;
    QProgressBar *m_xapianProgress;
    QTimer *m_xapianTimeout;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void updateStatus();

private Q_SLOTS:
    void showXapianProgress();
    void hideXapianProgress();
    void updateXapianProgress(int percentage);
};

#endif
