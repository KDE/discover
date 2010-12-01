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

#ifndef APPLICATIONDETAILSWIDGET_H
#define APPLICATIONDETAILSWIDGET_H

// Qt includes
#include <QtGui/QScrollArea>

#include <LibQApt/Globals>

class QLabel;
class QProgressBar;
class QPropertyAnimation;
class QPushButton;

class KJob;
class KPixmapSequenceOverlayPainter;
class KTemporaryFile;

class Application;
class ApplicationBackend;
class ClickableLabel;
class ScreenShotViewer;

// Widget for showing details about a single application
class ApplicationDetailsWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit ApplicationDetailsWidget(QWidget *parent, ApplicationBackend *backend);
    ~ApplicationDetailsWidget();

    void setApplication(Application *app);

private:
    Application *m_app;
    ApplicationBackend *m_appBackend;

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_shortDescLabel;
    QWidget *m_menuPathWidget;
    QLabel *m_menuPathLabel;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    QLabel *m_longDescLabel;
    ClickableLabel *m_screenshotLabel;
    QLabel *m_websiteLabel;
    QLabel *m_size;
    QLabel *m_version;
    QLabel *m_license;
    QLabel *m_support;

    QPropertyAnimation *m_fadeScreenshot;
    KPixmapSequenceOverlayPainter *m_throbberWidget;

    KTemporaryFile *m_screenshotFile;
    ScreenShotViewer *m_screenshotDialog;

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event, Application *app);
    void updateProgress(Application *app, int percentage);
    void transactionQueued(Application *app);
    void fadeInScreenshot();
    void fetchScreenshot(QApt::ScreenshotType screenshotType);
    void thumbnailFetched(KJob *job);
    void screenshotFetched(KJob *job);
    void screenshotLabelClicked();
    void onScreenshotDialogClosed();
    void actionButtonClicked();

Q_SIGNALS:
    void installButtonClicked(Application *app);
    void removeButtonClicked(Application *app);
};

#endif
