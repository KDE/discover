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

#ifndef DOWNLOADWIDGET_H
#define DOWNLOADWIDGET_H

// Qt includes
#include <QWidget>

class QLabel;
class QTreeView;
class QProgressBar;
class QPushButton;

class DownloadDelegate;
class DownloadModel;

namespace QApt {
    class Transaction;
}

class DownloadWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadWidget(QWidget *parent);

    void setTransaction(QApt::Transaction *trans);

private:
    QLabel *m_headerLabel;
    QTreeView *m_downloadView;
    DownloadModel *m_downloadModel;
    DownloadDelegate *m_downloadDelegate;
    QProgressBar *m_totalProgress;
    QLabel *m_downloadLabel;
    QPushButton *m_cancelButton;

public Q_SLOTS:
    void setHeaderText(const QString &text);
    void updateDownloadProgress(int percentage, int speed, int ETA);
    void updatePackageDownloadProgress(const QString &name, int percentage, const QString &URI, double size, int flag);
    void clear();

private Q_SLOTS:
    void cancelButtonPressed();

signals:
    void cancelDownload();
};

#endif
