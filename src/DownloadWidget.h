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

#include <QItemDelegate>
#include <QTextDocument>
#include <QWidget>

#include <KVBox>

class QLabel;
class QListView;
class QProgressBar;
class QStandardItemModel;
class QPushButton;

class DownloadWidget : public KVBox
{
    Q_OBJECT
public:
    DownloadWidget(QWidget *parent);

    ~DownloadWidget();

    void clear();
    void addItem(const QString &message);
    void setHeaderText(const QString &text);

private:
    QLabel *m_headerLabel;
    QListView *m_downloadView;
    QStandardItemModel *m_downloadModel;
    QProgressBar *m_totalProgress;
    QLabel *m_downloadLabel;
    QPushButton *m_cancelButton;

public Q_SLOTS:
    void updateDownloadProgress(int percentage, int speed, int ETA);
    void updateDownloadMessage(int flag, const QString &message);

private Q_SLOTS:
    void cancelButtonPressed();

signals:
    void cancelDownload();
};

#endif
