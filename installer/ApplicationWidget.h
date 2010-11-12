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

#ifndef APPLICATIONWIDGET_H
#define APPLICATIONWIDGET_H

// Qt includes
#include <QtGui/QScrollArea>
#include <LibQApt/Globals>

class QLabel;
class QPushButton;

class KJob;
class KTemporaryFile;

class Application;
class ClickableLabel;

// Widget for showing details about a single application
class ApplicationWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit ApplicationWidget(QWidget *parent, Application *app);
    ~ApplicationWidget();

private:
    Application *m_app;

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_shortDescLabel;
    QLabel *m_longDescLabel;
    ClickableLabel *m_screenshotLabel;
    QLabel *m_websiteLabel;
    QLabel *m_size;
    QLabel *m_version;
    QLabel *m_license;
    QLabel *m_support;

    KTemporaryFile *m_screenshotFile;

private Q_SLOTS:
    void fetchScreenshot(QApt::ScreenshotType screenshotType);
    void thumbnailFetched(KJob *job);
    void screenshotFetched(KJob *job);
    void screenshotLabelClicked();
};

#endif
