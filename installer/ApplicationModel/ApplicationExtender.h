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

#ifndef APPLICATIONEXTENDER_H
#define APPLICATIONEXTENDER_H

#include <QtGui/QWidget>

#include <LibQApt/Globals>

class QProgressBar;
class QPushButton;

class Application;

namespace QApt {
    class Backend;
}

class ApplicationExtender : public QWidget
{
    Q_OBJECT
public:
    ApplicationExtender(QWidget *parent, Application *app, QApt::Backend *backend);
    ~ApplicationExtender();

private:
    Application *m_app;
    QApt::Backend *m_backend;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event);
    void updateProgress(int percentage);
    void updateProgress(const QString &text, int percentage);
    void emitInfoButtonClicked();
    void emitRemoveButtonClicked();
    void emitInstallButtonClicked();

Q_SIGNALS:
    void infoButtonClicked(Application *app);
    void removeButtonClicked(Application *app);
    void installButtonClicked(Application *app);
};

#endif
