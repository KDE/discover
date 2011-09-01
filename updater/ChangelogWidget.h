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

#ifndef CHANGELOGWIDGET_H
#define CHANGELOGWIDGET_H

#include <QtGui/QWidget>

class QParallelAnimationGroup;

class KJob;
class KPixmapSequenceOverlayPainter;
class KTemporaryFile;
class KTextBrowser;

namespace QApt {
    class Backend;
    class Changelog;
    class Package;
}

class ChangelogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChangelogWidget(QWidget *parent = 0);

private:
    QApt::Backend *m_backend;
    QApt::Package *m_package;
    QString m_jobFileName;
    bool m_show;

    QParallelAnimationGroup *m_expandWidget;
    KTextBrowser *m_changelogBrowser;
    KPixmapSequenceOverlayPainter *m_busyWidget;

    QString buildDescription(const QApt::Changelog &log);

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void setPackage(QApt::Package *package);
    void show();
    void animatedHide();

private Q_SLOTS:
    void fetchChangelog();
    void changelogFetched(KJob *job);
};

#endif
