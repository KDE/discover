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

// LibQApt includes
#include <LibQApt/Globals>

// Own includes
#include "ApplicationBackend.h"
#include "Transaction.h"

class QLabel;
class QProgressBar;
class QPropertyAnimation;
class QPushButton;

class KJob;
class KPixmapSequenceOverlayPainter;
class KRatingWidget;
class KTemporaryFile;

class AddonsWidget;
class Application;
class ClickableLabel;
class Review;
class ReviewsWidget;
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
    KRatingWidget *m_ratingWidget;
    QLabel *m_ratingCountLabel;
    QWidget *m_menuPathWidget;
    QLabel *m_menuPathLabel;
    QLabel *m_statusLabel;
    QLabel *m_usageLabel;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    QPushButton *m_cancelButton;
    QLabel *m_longDescLabel;
    ClickableLabel *m_screenshotLabel;
    QLabel *m_websiteLabel;
    AddonsWidget *m_addonsWidget;
    QLabel *m_size;
    QLabel *m_version;
    QLabel *m_license;
    QLabel *m_support;
    ReviewsWidget *m_reviewsWidget;

    QPropertyAnimation *m_fadeScreenshot;
    KPixmapSequenceOverlayPainter *m_throbberWidget;

    KTemporaryFile *m_screenshotFile;
    ScreenShotViewer *m_screenshotDialog;

private Q_SLOTS:
    void workerEvent(QApt::WorkerEvent event, Transaction *transaction);
    void updateProgress(Transaction *transaction, int percentage);
    void showTransactionState(Transaction *transaction);
    void transactionCancelled(Application *app);
    void populateZeitgeistInfo();
    void fadeInScreenshot();
    void fetchScreenshot(QApt::ScreenshotType screenshotType);
    void thumbnailFetched(KJob *job);
    void screenshotFetched(KJob *job);
    void screenshotLabelClicked();
    void onScreenshotDialogClosed();
    void actionButtonClicked();
    void cancelButtonClicked();
    void populateAddons();
    void populateReviews(Application *app, const QList<Review *> &reviews);
    void addonsApplyButtonClicked(const QHash<QApt::Package *, QApt::Package::State> &changedAddons);

Q_SIGNALS:
    void installButtonClicked(Application *app);
    void installButtonClicked(Application *app, const QHash<QApt::Package *, QApt::Package::State> &);
    void removeButtonClicked(Application *app);
    void cancelButtonClicked(Application *app);
};

#endif
