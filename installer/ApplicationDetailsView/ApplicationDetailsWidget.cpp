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

#include "ApplicationDetailsWidget.h"

#include <QApplication>
#include <QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItemModel>
#include <QtGui/QVBoxLayout>

#include <KGlobal>
#include <KHBox>
#include <KIcon>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KPixmapSequence>
#include <kpixmapsequenceoverlaypainter.h>
#include <KService>
#include <KStandardGuiItem>
#include <KTemporaryFile>
#include <KToolInvocation>
#include <KDebug>
#include <Nepomuk/KRatingWidget>

#include <LibQApt/Package>


#include "HaveQZeitgeist.h"
#ifdef HAVE_QZEITGEIST
#include <QtZeitgeist/DataModel/Event>
#include <QtZeitgeist/DataModel/TimeRange>
#include <QtZeitgeist/Log>
#include <QtZeitgeist/Interpretation>
#include <QtZeitgeist/Manifestation>
#include <QtZeitgeist/QtZeitgeist>
#endif

#include <math.h>

#include "Application.h"
#include "ClickableLabel.h"
#include "effects/GraphicsOpacityDropShadowEffect.h"
#include "ScreenShotViewer.h"
#include "ReviewsBackend/Rating.h"
#include "ReviewsBackend/Review.h"
#include "ReviewsBackend/ReviewsWidget.h"
#include "ReviewsBackend/ReviewsBackend.h"
#include "../../libmuon/MuonStrings.h"

#define BLUR_RADIUS 15

ApplicationDetailsWidget::ApplicationDetailsWidget(QWidget *parent, ApplicationBackend *backend)
    : QScrollArea(parent)
    , m_appBackend(backend)
    , m_screenshotFile(0)
    , m_screenshotDialog(0)
{
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    viewport()->setAutoFillBackground(false);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    widget->setLayout(layout);

    // Header
    QWidget *headerWidget = new QWidget(widget);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerWidget->setLayout(headerLayout);

    m_iconLabel = new QLabel(headerWidget);

    QWidget *nameDescWidget = new QWidget(headerWidget);
    QVBoxLayout *nameDescLayout = new QVBoxLayout(nameDescWidget);
    m_nameLabel = new QLabel(nameDescWidget);
    m_nameLabel->setAlignment(Qt::AlignLeft);
    m_shortDescLabel = new QLabel(nameDescWidget);

    nameDescLayout->addWidget(m_nameLabel);
    nameDescLayout->addWidget(m_shortDescLabel);

    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QWidget *ratingUseWidget = new QWidget(headerWidget);
    QVBoxLayout *ratingUseLayout = new QVBoxLayout(ratingUseWidget);

    m_ratingWidget = new KRatingWidget(ratingUseWidget);
    m_ratingWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_ratingWidget->setPixmapSize(32);

    m_ratingCountLabel = new QLabel(ratingUseWidget);
    m_ratingCountLabel->setAlignment(Qt::AlignHCenter);

    ratingUseLayout->addWidget(m_ratingWidget);
    ratingUseLayout->addWidget(m_ratingCountLabel);

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(nameDescWidget);
    headerLayout->addWidget(headerSpacer);
    headerLayout->addWidget(ratingUseWidget);

    // Menu path label
    m_menuPathWidget = new QWidget(this);
    QHBoxLayout *menuPathLayout = new QHBoxLayout(m_menuPathWidget);
    m_menuPathWidget->setLayout(menuPathLayout);

    QLabel *menuLabel = new QLabel(m_menuPathWidget);
    menuLabel->setText(i18nc("@info", "Find in the menu:"));
    m_menuPathLabel = new QLabel(m_menuPathWidget);

    QWidget *menuPathSpacer = new QWidget(this);
    menuPathSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    menuPathLayout->addWidget(menuLabel);
    menuPathLayout->addWidget(m_menuPathLabel);
    menuPathLayout->addWidget(menuPathSpacer);

    // Install/remove/update button
    QFrame *actionButtonWidget = new QFrame(this);
    QHBoxLayout *actionButtonLayout = new QHBoxLayout(actionButtonWidget);
    actionButtonWidget->setLayout(actionButtonLayout);
    actionButtonWidget->setFrameShadow(QFrame::Sunken);
    actionButtonWidget->setFrameShape(QFrame::StyledPanel);

    m_statusLabel = new QLabel(actionButtonWidget);
    m_usageLabel = new QLabel(actionButtonWidget);

    QWidget *actionButtonSpacer = new QWidget(actionButtonWidget);
    actionButtonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_actionButton = new QPushButton(actionButtonWidget);
    connect(m_actionButton, SIGNAL(clicked()), this, SLOT(actionButtonClicked()));

    m_progressBar = new QProgressBar(actionButtonWidget);
    m_progressBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_progressBar->hide();

    m_cancelButton = new QPushButton(actionButtonWidget);
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    m_cancelButton->setIcon(cancelButton.icon());
    m_cancelButton->setToolTip(cancelButton.toolTip());
    m_cancelButton->hide();
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));

    actionButtonLayout->addWidget(m_statusLabel);
    actionButtonLayout->addWidget(m_usageLabel);
    actionButtonLayout->addWidget(actionButtonSpacer);
    actionButtonLayout->addWidget(m_actionButton);
    actionButtonLayout->addWidget(m_progressBar);
    actionButtonLayout->addWidget(m_cancelButton);

    // Long description and Screenshot
    QWidget *body = new QWidget(widget);
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);

    m_longDescLabel = new QLabel(body);
    m_longDescLabel->setWordWrap(true);
    m_longDescLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_longDescLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_screenshotLabel = new ClickableLabel(body);
    m_screenshotLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_screenshotLabel->setMinimumSize(170, 130);

    m_throbberWidget = new KPixmapSequenceOverlayPainter(m_screenshotLabel);
    m_throbberWidget->setSequence(KPixmapSequence("process-working", 22));
    m_throbberWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_throbberWidget->setWidget(m_screenshotLabel);
    m_throbberWidget->start();

    connect(m_screenshotLabel, SIGNAL(clicked()), this, SLOT(screenshotLabelClicked()));

    bodyLayout->addWidget(m_longDescLabel);
    bodyLayout->addWidget(m_screenshotLabel);

    m_addonsWidget = new QWidget(widget);
    QVBoxLayout *addonsLayout = new QVBoxLayout(m_addonsWidget);
    m_addonsWidget->setLayout(addonsLayout);
    m_addonsWidget->hide();

    QLabel *addonsTitle = new QLabel(m_addonsWidget);
    addonsTitle->setText(i18nc("@title:group", "Addons"));
    addonsTitle->setAlignment(Qt::AlignLeft);

    m_addonsView = new QListView(m_addonsWidget);
    m_addonsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_addonsModel = new QStandardItemModel(this);
    m_addonsView->setModel(m_addonsModel);

    QWidget *addonsButtonBox = new QWidget(m_addonsWidget);
    QHBoxLayout *addonButtonsLayout = new QHBoxLayout(addonsButtonBox);

    QWidget *addonsButtonSpacer = new QWidget(m_addonsWidget);
    addonsButtonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_addonsRevertButton = new QPushButton(addonsButtonBox);
    m_addonsRevertButton->setIcon(KIcon("edit-undo"));
    m_addonsRevertButton->setText(i18nc("@action:button", "Revert"));
    connect(m_addonsRevertButton, SIGNAL(clicked()),
            this, SLOT(addonsRevertButtonClicked()));

    m_addonsApplyButton = new QPushButton(m_addonsWidget);
    m_addonsApplyButton->setIcon(KIcon("dialog-ok-apply"));
    m_addonsApplyButton->setText(i18nc("@action:button", "Apply"));
    m_addonsApplyButton->setToolTip(i18nc("@info:tooltip", "Apply changes to addons"));
    connect(m_addonsApplyButton, SIGNAL(clicked()),
            this, SLOT(addonsApplyButtonClicked()));

    addonButtonsLayout->addWidget(addonsButtonSpacer);
    addonButtonsLayout->addWidget(m_addonsRevertButton);
    addonButtonsLayout->addWidget(m_addonsApplyButton);

    addonsLayout->addWidget(addonsTitle);
    addonsLayout->addWidget(m_addonsView);
    addonsLayout->addWidget(addonsButtonBox);

    m_websiteLabel = new QLabel(this);
    m_websiteLabel->setAlignment(Qt::AlignLeft);
    m_websiteLabel->setOpenExternalLinks(true);

    // Technical details
    QWidget *detailsWidget = new QWidget(widget);
    QGridLayout *detailsGrid = new QGridLayout(detailsWidget);

    // detailsGrid, row 0
    QLabel *sizeLabel = new QLabel(detailsWidget);
    sizeLabel->setText(i18nc("@label Label preceding the app size", "Total Size:"));
    m_size = new QLabel(detailsWidget);
    detailsGrid->addWidget(sizeLabel, 0, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_size, 0, 1, Qt::AlignLeft);

    // detailsGrid, row 1
    QLabel *versionLabel = new QLabel(detailsWidget);
    versionLabel->setText(i18nc("@label Label preceding the app version", "Version:"));
    m_version = new QLabel(detailsWidget);
    detailsGrid->addWidget(versionLabel, 1, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_version, 1, 1, Qt::AlignLeft);

    // detailsGrid, row 2
    QLabel *licenseLabel = new QLabel(detailsWidget);
    licenseLabel->setText(i18nc("@label Label preceding the app license", "License:"));
    m_license = new QLabel(detailsWidget);
    detailsGrid->addWidget(licenseLabel, 2, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_license, 2, 1, Qt::AlignLeft);

    // detailsGrid, row 3
    QLabel *supportLabel = new QLabel(detailsWidget);
    supportLabel->setText(i18nc("@label Label preceding the app support", "Support:"));
    m_support = new QLabel(detailsWidget);
    detailsGrid->addWidget(supportLabel, 3, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_support, 3, 1, Qt::AlignLeft);

    detailsGrid->setColumnStretch(1,1);

    m_reviewsWidget = new ReviewsWidget(widget);

    QWidget *verticalSpacer = new QWidget(this);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);


    layout->addWidget(headerWidget);
    layout->addWidget(m_menuPathWidget);
    layout->addWidget(actionButtonWidget);
    layout->addWidget(body);
    layout->addWidget(m_addonsWidget);
    layout->addWidget(m_websiteLabel);
    layout->addWidget(detailsWidget);
    layout->addWidget(m_reviewsWidget);
    layout->addWidget(verticalSpacer);

    connect(m_appBackend, SIGNAL(workerEvent(QApt::WorkerEvent, Transaction *)),
            this, SLOT(workerEvent(QApt::WorkerEvent, Transaction *)));
    connect(m_appBackend, SIGNAL(transactionCancelled(Application *)),
            this, SLOT(transactionCancelled(Application *)));

    setWidget(widget);
}

ApplicationDetailsWidget::~ApplicationDetailsWidget()
{
    delete m_screenshotFile;
}

void ApplicationDetailsWidget::setApplication(Application *app)
{
    m_app = app;

    // FIXME: Always keep label size at 48x48, and render the largest size
    // we can up to that point. Otherwise some icons will be blurry
    m_iconLabel->setPixmap(KIcon(app->icon()).pixmap(48,48));

    m_nameLabel->setText(QLatin1Literal("<h1>") % app->name() % QLatin1Literal("</h1>"));
    m_shortDescLabel->setText(app->comment());

    ReviewsBackend *reviewsBackend = m_appBackend->reviewsBackend();
    Rating *rating = reviewsBackend->ratingForApplication(app);
    if (rating) {
        m_ratingWidget->setRating(rating->rating());
        m_ratingCountLabel->setText(i18ncp("@label The number of ratings the app has",
                                           "%1 rating", "%1 ratings",
                                           rating->ratingCount()));
    } else {
        m_ratingWidget->hide();
        m_ratingCountLabel->hide();
    }

    populateZeitgeistInfo();

    QString menuPathString = app->menuPath();
    if (!menuPathString.isEmpty()) {
        m_menuPathLabel->setText(menuPathString);
    } else {
        m_menuPathWidget->hide();
    }

    if (!app->package()->isInstalled()) {
        m_statusLabel->setText(MuonStrings::global()->packageStateName(QApt::Package::NotInstalled));
        m_actionButton->setText(i18nc("@action", "Install"));
        m_actionButton->setIcon(KIcon("download"));
        m_actionButton->show();
    } else {
        m_statusLabel->setText(MuonStrings::global()->packageStateName(QApt::Package::Installed));
        m_actionButton->setText(i18nc("@action", "Remove"));
        m_actionButton->setIcon(KIcon("edit-delete"));
    }

    m_longDescLabel->setText(app->package()->longDescription());

    populateAddons();

    QString homepageUrl = app->package()->homepage();
    if (!homepageUrl.isEmpty()) {
        QString websiteString = i18nc("@label visible text for an app's URL", "Website");
        m_websiteLabel->setText(QLatin1Literal("<a href=\"") % homepageUrl % "\">" %
                                websiteString % QLatin1Literal("</a>"));
        m_websiteLabel->setToolTip(homepageUrl);
    } else {
        m_websiteLabel->hide();
    }

    if (!app->package()->isInstalled()) {
        m_size->setText(i18nc("@info app size", "%1 to download, %2 on disk",
                              KGlobal::locale()->formatByteSize(app->package()->downloadSize()),
                              KGlobal::locale()->formatByteSize(app->package()->availableInstalledSize())));
    } else {
        m_size->setText(i18nc("@info app size", "%1 on disk",
                              KGlobal::locale()->formatByteSize(app->package()->currentInstalledSize())));
    }

    if (!app->package()->isInstalled()) {
         m_version->setText(app->package()->availableVersion());
    } else {
         m_version->setText(app->package()->installedVersion());
    }

    if (app->package()->component() == "main" ||
        app->package()->component() == "universe") {
        m_license->setText(i18nc("@info license", "Open Source"));
    } else if (app->package()->component() == "restricted") {
        m_license->setText(i18nc("@info license", "Proprietary"));
    }

    if (app->package()->isSupported()) {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical provides critical updates for %1 until %2",
                                 app->name(), app->package()->supportedUntil()));
    } else {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical does not provide updates for %1. Some updates "
                                 "may be provided by the Ubuntu community", app->name()));
    }

    // Fetch reviews
    reviewsBackend->fetchReviews(app);
    connect(reviewsBackend, SIGNAL(reviewsReady(Application *, QList<Review *>)),
            this, SLOT(populateReviews(Application *, QList<Review *>)));

    // Catch already-begun downloads. If the state is something else, we won't
    // care because we won't handle it
    QPair<QApt::WorkerEvent, Transaction *> workerState = m_appBackend->workerState();
    workerEvent(workerState.first, workerState.second);

    foreach (Transaction *transaction, m_appBackend->transactions()) {
        if (transaction->application() == m_app){
            showTransactionState(transaction);
        }
    }

    fetchScreenshot(QApt::Thumbnail);
}

void ApplicationDetailsWidget::workerEvent(QApt::WorkerEvent event, Transaction *transaction)
{
    if (!transaction || m_app != transaction->application()) {
        return;
    }

    switch (event) {
    case QApt::PackageDownloadStarted:
        m_actionButton->hide();
        m_cancelButton->show();
        m_progressBar->show();
        m_progressBar->setFormat(i18nc("@info:status", "Downloading"));
        connect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::PackageDownloadFinished:
        disconnect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                   this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::CommitChangesStarted:
        m_actionButton->hide();
        m_cancelButton->hide();
        m_progressBar->show();
        m_progressBar->setValue(0);
        switch(transaction->action()) {
        case InstallApp:
            m_progressBar->setFormat(i18nc("@info:status", "Installing"));
            break;
        case ChangeAddons:
            m_progressBar->setFormat(i18nc("@info:status", "Changing Addons"));
            break;
        case RemoveApp:
            m_progressBar->setFormat(i18nc("@info:status", "Removing"));
            break;
        default:
            break;
        }
        connect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                this, SLOT(updateProgress(Transaction *, int)));
        break;
    case QApt::CommitChangesFinished:
        disconnect(m_appBackend, SIGNAL(progress(Transaction *, int)),
                   this, SLOT(updateProgress(Transaction *, int)));
        break;
    default:
        break;
    }
}

void ApplicationDetailsWidget::updateProgress(Transaction *transaction, int percentage)
{
    if (m_app == transaction->application()) {
        m_progressBar->setValue(percentage);

        if (percentage == 100) {
            m_progressBar->setFormat(i18nc("@info:status Progress text when done", "Done"));
        }
    }
}

void ApplicationDetailsWidget::showTransactionState(Transaction *transaction)
{
    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);

    QString text;
    switch (transaction->state()) {
    case QueuedState:
        text = i18nc("@info:status Progress text when waiting", "Waiting");
        break;
    case RunningState:
        switch (transaction->action()) {
        case InstallApp:
            text = i18nc("@info:status", "Installing");
            break;
        case ChangeAddons:
            text = i18nc("@info:status", "Changing Addons");
            break;
        case RemoveApp:
            text = i18nc("@info:status", "Removing");
            break;
        default:
            break;
        }
        break;
    case DoneState:
        text = i18nc("@info:status Progress text when done", "Done");
        m_progressBar->setValue(100);
        break;
    default:
        break;
    }
    m_progressBar->setFormat(text);
}

void ApplicationDetailsWidget::transactionCancelled(Application *app)
{
    if (m_app == app) {
        m_progressBar->hide();
        m_cancelButton->hide();
        m_actionButton->show();
    }
}

void ApplicationDetailsWidget::populateZeitgeistInfo()
{
#ifdef HAVE_QZEITGEIST
    if (!m_app->package()->isInstalled()) {
        m_usageLabel->hide();
        return;
    }

    QString desktopFile;

    foreach (const QString &desktop, m_app->package()->installedFilesList().filter(".desktop")) {
            KService::Ptr service = KService::serviceByDesktopPath(desktop);
            if (!service) {
                break;
            }

            if (service->isApplication() &&
              !service->noDisplay() &&
              !service->exec().isEmpty())
            {
                desktopFile = desktop.split('/').last();
                break;
            }
    }

    QtZeitgeist::init();

    // Prepare the Zeitgeist query
    QtZeitgeist::DataModel::EventList eventListTemplate;

    QtZeitgeist::DataModel::Event event1;
    event1.setActor("application://" + desktopFile);
    event1.setInterpretation(QtZeitgeist::Interpretation::Event::ZGModifyEvent);
    event1.setManifestation(QtZeitgeist::Manifestation::Event::ZGUserActivity);

    QtZeitgeist::DataModel::Event event2;
    event2.setActor("application://" + desktopFile);
    event2.setInterpretation(QtZeitgeist::Interpretation::Event::ZGCreateEvent);
    event2.setManifestation(QtZeitgeist::Manifestation::Event::ZGUserActivity);

    eventListTemplate << event1 << event2;

    QtZeitgeist::Log log;
    QDBusPendingReply<QtZeitgeist::DataModel::EventIdList> reply = log.findEventIds(QtZeitgeist::DataModel::TimeRange::always(),
                                                                                    eventListTemplate,
                                                                                    QtZeitgeist::Log::Any,
                                                                                    0, QtZeitgeist::Log::MostRecentEvents);
    reply.waitForFinished();

    int usageCount = 0;
    if (reply.isValid()) {
        usageCount = reply.value().size();
    }

    if (!usageCount) {
        // More likely there are no stats than no usage, so just hide the label and return
        m_usageLabel->hide();
        return;
    }

    m_usageLabel->setText(i18ncp("@label The number of times an app has been used",
                                  "Used one time", "(Used %1 times)", usageCount));
#else
    m_usageLabel->hide();
#endif // HAVE_QZEITGEIST
}

void ApplicationDetailsWidget::fadeInScreenshot()
{
    m_fadeScreenshot->setDirection(QAbstractAnimation::Forward);
    m_fadeScreenshot->start();
}

void ApplicationDetailsWidget::fetchScreenshot(QApt::ScreenshotType screenshotType)
{
    m_screenshotLabel->setCursor(Qt::BusyCursor);
    if (m_screenshotFile) {
        m_screenshotFile->deleteLater();
        m_screenshotFile = 0;
    }
    m_screenshotFile = new KTemporaryFile;
    m_screenshotFile->setPrefix("muon");
    m_screenshotFile->setSuffix(".png");
    m_screenshotFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_app->screenshotUrl(screenshotType),
                               m_screenshotFile->fileName(), -1, KIO::Overwrite | KIO::HideProgressInfo);

    switch (screenshotType) {
    case QApt::Thumbnail:
        connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(thumbnailFetched(KJob *)));
        break;
    case QApt::Screenshot:
        connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(screenshotFetched(KJob *)));
        break;
    case QApt::UnknownType:
    default:
        break;
    }
}

void ApplicationDetailsWidget::thumbnailFetched(KJob *job)
{
    m_throbberWidget->stop();
    if (job->error()) {
        m_screenshotLabel->hide();
        return;
    }

    GraphicsOpacityDropShadowEffect *shadow = new GraphicsOpacityDropShadowEffect(m_screenshotLabel);
    shadow->setBlurRadius(BLUR_RADIUS);
    shadow->setOpacity(0);
    shadow->setOffset(2);
    shadow->setColor(QApplication::palette().dark().color());
    m_screenshotLabel->setGraphicsEffect(shadow);

    m_fadeScreenshot = new QPropertyAnimation(shadow, "opacity", this);
    m_fadeScreenshot->setDuration(500);
    m_fadeScreenshot->setStartValue(qreal(0));
    m_fadeScreenshot->setEndValue(qreal(1));

    m_screenshotLabel->setPixmap(QPixmap(m_screenshotFile->fileName()).scaled(160,120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_screenshotLabel->setCursor(Qt::PointingHandCursor);
    fadeInScreenshot();
}

void ApplicationDetailsWidget::screenshotFetched(KJob *job)
{
    m_screenshotLabel->unsetCursor();
    if (job->error()) {
        return;
    }

    m_screenshotDialog = new ScreenShotViewer(m_screenshotFile->fileName());
    connect(m_screenshotDialog, SIGNAL(destroyed(QObject *)),
            this, SLOT(onScreenshotDialogClosed()));
    connect(m_screenshotDialog, SIGNAL(finished(int)),
            this, SLOT(onScreenshotDialogClosed()));
    m_screenshotDialog->setWindowTitle(m_app->name());
    m_screenshotDialog->show();
}

void ApplicationDetailsWidget::screenshotLabelClicked()
{
    fetchScreenshot(QApt::Screenshot);
}

void ApplicationDetailsWidget::onScreenshotDialogClosed()
{
    m_screenshotLabel->setCursor(Qt::PointingHandCursor);
    m_screenshotDialog->deleteLater();
    m_screenshotDialog = 0;
}

void ApplicationDetailsWidget::actionButtonClicked()
{
    // TODO: update packages
    if (m_app->package()->isInstalled()) {
        emit removeButtonClicked(m_app);
    } else {
        emit installButtonClicked(m_app);
    }

    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status Progress text when waiting", "Waiting"));
}

void ApplicationDetailsWidget::cancelButtonClicked()
{
    emit cancelButtonClicked(m_app);
}

void ApplicationDetailsWidget::populateAddons()
{
    QApt::PackageList addons = m_app->addons();

    m_availableAddons = addons;

    if (!addons.isEmpty()) {
        m_addonsWidget->show();
        connect(m_addonsModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                this, SLOT(addonStateChanged(const QModelIndex &, const QModelIndex &)));
    }

    foreach (QApt::Package *addon, addons) {
        // Check if we have an application for the addon
        Application *addonApp = 0;

        foreach (Application *app, m_appBackend->applicationList()) {
            if (app->package()->name() == addon->name()) {
                addonApp = app;
            }
        }

        QStandardItem *addonItem = new QStandardItem;
        addonItem->setData(addon->latin1Name());
        QString packageName = QLatin1Literal(" (") % addon->latin1Name() % ')';
        if (addonApp) {
            addonItem->setText(addonApp->name() % packageName);
            addonItem->setIcon(KIcon(addonApp->icon()));
        } else {
            addonItem->setText(addon->shortDescription() % packageName);
            addonItem->setIcon(KIcon("applications-other"));
        }

        addonItem->setEditable(false);
        addonItem->setCheckable(true);

        if (addon->isInstalled()) {
            addonItem->setCheckState(Qt::Checked);
        } else {
            addonItem->setCheckState(Qt::Unchecked);
        }

        m_addonsModel->appendRow(addonItem);
    }

    m_addonsRevertButton->setEnabled(false);
    m_addonsApplyButton->setEnabled(false);
}

void ApplicationDetailsWidget::populateReviews(Application *app, const QList<Review *> &reviews)
{
    if (reviews.isEmpty() || app != m_app) {
        return;
    }

    m_reviewsWidget->addReviews(reviews);
}

void ApplicationDetailsWidget::addonStateChanged(const QModelIndex &left, const QModelIndex &right)
{
    Q_UNUSED(right);
    QStandardItem *item = m_addonsModel->itemFromIndex(left);
    QApt::Package *addon = 0;

    QString addonName = item->data().toString();
    foreach(QApt::Package *pkg, m_availableAddons) {
        if (pkg->latin1Name() == addonName) {
            addon = pkg;
        }
    }

    if (!addon) {
        return;
    }

    if (addon->isInstalled()) {
        switch (item->checkState()) {
        case Qt::Checked:
            if (m_changedAddons.contains(addon)) {
                m_changedAddons.remove(addon);
            }
            break;
        case Qt::Unchecked:
            m_changedAddons[addon] = QApt::Package::ToRemove;
            break;
        default:
            break;
        }
    } else {
        switch (item->checkState()) {
        case Qt::Checked:
            m_changedAddons[addon] = QApt::Package::ToInstall;
            break;
        case Qt::Unchecked:
            if (m_changedAddons.contains(addon)) {
                m_changedAddons.remove(addon);
            }
            break;
        default:
            break;
        }
    }

    m_addonsRevertButton->setEnabled(!m_changedAddons.isEmpty());
    m_addonsApplyButton->setEnabled(!m_changedAddons.isEmpty());
}

void ApplicationDetailsWidget::addonsRevertButtonClicked()
{
    m_availableAddons.clear();
    m_addonsModel->clear();
    populateAddons();
}

void ApplicationDetailsWidget::addonsApplyButtonClicked()
{
    emit installButtonClicked(m_app, m_changedAddons);

    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status Progress text when waiting", "Waiting"));
}

#include "ApplicationDetailsWidget.moc"
