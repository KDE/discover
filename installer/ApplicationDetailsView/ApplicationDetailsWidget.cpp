/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@kde.org>                *
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

// Qt includes
#include <QApplication>
#include <QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeError>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGraphicsDropShadowEffect>
#include <QtGui/QGraphicsBlurEffect>
#include <QtGui/QGraphicsObject>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KDialog>
#include <KGlobal>
#include <KHBox>
#include <KIcon>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KPixmapSequence>
#include <kpixmapsequenceoverlaypainter.h>
#include <KService>
#include <KStandardDirs>
#include <KStandardGuiItem>
#include <KTemporaryFile>
#include <KToolInvocation>
#include <KDebug>
#include <Nepomuk/KRatingWidget>

// LibQApt includes
#include <LibQApt/Package>

// QZeitgeist includes
#include "HaveQZeitgeist.h"

// Libmuon includes
#include <Application.h>
#include <MuonStrings.h>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <ReviewsBackend/ReviewsBackend.h>
#include <Transaction/TransactionListener.h>
#include "../../libmuon/mobile/src/mousecursor.h"

// std includes
#include <math.h>

// Own includes
#include "AddonsWidget.h"
#include "ClickableLabel.h"
#include "ReviewsWidget/ReviewsWidget.h"
#include "ScreenShotOverlay.h"

#define BLUR_RADIUS 15

ApplicationDetailsWidget::ApplicationDetailsWidget(QWidget *parent, ApplicationBackend *backend)
    : QScrollArea(parent)
    , m_appBackend(backend)
    , m_screenshotFile(0)
{
    qmlRegisterType<QGraphicsDropShadowEffect>("Effects",1,0,"DropShadow");
    qmlRegisterType<MouseCursor>("MuonMobile", 1, 0, "MouseCursor");

    setWidgetResizable(true);
    viewport()->setAutoFillBackground(false);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    widget->setLayout(layout);
    widget->setBackgroundRole(QPalette::Base);

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
    connect(m_appBackend, SIGNAL(reloadFinished()), this, SLOT(updateActionButton()));

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

    KVBox *bodyLeft = new KVBox(body);
    bodyLeft->setSpacing(2*KDialog::spacingHint());

    m_longDescLabel = new QLabel(bodyLeft);
    m_longDescLabel->setWordWrap(true);
    m_longDescLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_longDescLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_websiteLabel = new QLabel(bodyLeft);
    m_websiteLabel->setAlignment(Qt::AlignLeft);
    m_websiteLabel->setOpenExternalLinks(true);

    m_screenshotView = new QDeclarativeView(this);
    m_screenshotView->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_screenshotView->setMinimumSize(170, 130);
    m_screenshotView->rootContext()->setContextProperty("view", m_screenshotView);
    m_screenshotView->setSource(KStandardDirs::locate("data", QLatin1String("libmuon/ThumbnailView.qml")));
    QObject *item = m_screenshotView->rootObject();
    connect(item, SIGNAL(thumbnailClicked()), this, SLOT(screenshotLabelClicked()));

    m_throbberWidget = new KPixmapSequenceOverlayPainter(m_screenshotView->viewport());
    m_throbberWidget->setSequence(KPixmapSequence("process-working", 22));
    m_throbberWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_throbberWidget->setWidget(m_screenshotView->viewport());
    m_throbberWidget->start();
    connect(item, SIGNAL(thumbnailLoaded()), m_throbberWidget, SLOT(stop()));

    bodyLayout->addWidget(bodyLeft);
    bodyLayout->addWidget(m_screenshotView);
    m_screenshotView->show();

    m_addonsWidget = new AddonsWidget(widget, m_appBackend);
    connect(m_addonsWidget, SIGNAL(applyButtonClicked(QHash<QApt::Package*,QApt::Package::State>)),
            this, SLOT(addonsApplyButtonClicked(QHash<QApt::Package*,QApt::Package::State>)));
    m_addonsWidget->hide();

    // Technical details
    QWidget *detailsWidget = new QWidget(widget);
    QGridLayout *detailsGrid = new QGridLayout(detailsWidget);

    // detailsGrid, row 0
    QLabel *sizeLabel = new QLabel(detailsWidget);
    sizeLabel->setText("<b>" % i18nc("@label Label preceding the app size", "Total Size:")  % "</b>");
    m_size = new QLabel(detailsWidget);
    detailsGrid->addWidget(sizeLabel, 0, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_size, 0, 1, Qt::AlignLeft);

    // detailsGrid, row 1
    QLabel *versionLabel = new QLabel(detailsWidget);
    versionLabel->setText("<b>" % i18nc("@label/rich Label preceding the app version", "Version:") % "</b>");
    m_version = new QLabel(detailsWidget);
    detailsGrid->addWidget(versionLabel, 1, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_version, 1, 1, Qt::AlignLeft);

    // detailsGrid, row 2
    QLabel *licenseLabel = new QLabel(detailsWidget);
    licenseLabel->setText("<b>" % i18nc("@label Label preceding the app license", "License:") % "</b>");
    m_license = new QLabel(detailsWidget);
    detailsGrid->addWidget(licenseLabel, 2, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_license, 2, 1, Qt::AlignLeft);

    // detailsGrid, row 3
    QLabel *supportLabel = new QLabel(detailsWidget);
    supportLabel->setText("<b>" % i18nc("@label Label preceding the app support", "Support:") % "</b>");
    m_support = new QLabel(detailsWidget);
    detailsGrid->addWidget(supportLabel, 3, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_support, 3, 1, Qt::AlignLeft);

    detailsGrid->setColumnStretch(1,1);

    m_reviewsWidget = new ReviewsWidget(widget);

    QWidget *verticalSpacer = new QWidget(this);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Blur effect for later
    m_blurEffect = new QGraphicsBlurEffect(widget);
    m_blurEffect->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
    m_blurEffect->setBlurRadius(0);
    widget->setGraphicsEffect(m_blurEffect);

    m_fadeBlur = new QPropertyAnimation(m_blurEffect, "blurRadius", this);
    m_fadeBlur->setDuration(200);
    m_fadeBlur->setStartValue(qreal(0));
    m_fadeBlur->setEndValue(qreal(5));

    // Workarounds for QScrollAreas not repainting whilst under the effects of a QGraphicsEffect
    connect(m_fadeBlur, SIGNAL(finished()), m_addonsWidget, SLOT(repaintViewport()));
    connect(m_fadeBlur, SIGNAL(finished()), m_screenshotView->viewport(), SLOT(repaint()));

    layout->addWidget(headerWidget);
    layout->addWidget(m_menuPathWidget);
    layout->addWidget(actionButtonWidget);
    layout->addWidget(body);
    layout->addWidget(m_addonsWidget);
    layout->addWidget(detailsWidget);
    layout->addWidget(m_reviewsWidget);
    layout->addWidget(verticalSpacer);
    
    m_listener = new TransactionListener(this);
    m_listener->setBackend(m_appBackend);

    connect(m_listener, SIGNAL(progressChanged()), SLOT(progressChanged()));
    connect(m_listener, SIGNAL(commentChanged()), SLOT(progressCommentChanged()));
    connect(m_listener, SIGNAL(running(bool)), SLOT(applicationRunningChanged(bool)));
    connect(m_listener, SIGNAL(downloading(bool)), SLOT(applicationDownloadingChanged(bool)));

    setWidget(widget);
}

ApplicationDetailsWidget::~ApplicationDetailsWidget()
{
    delete m_screenshotFile;
}

void ApplicationDetailsWidget::setApplication(Application *app)
{
    m_app = app;
    m_listener->setApplication(m_app);

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

#ifdef HAVE_QZEITGEIST
    m_usageLabel->setText(i18ncp("@label The number of times an app has been used",
                                  "Used one time", "(Used %1 times)", app->usageCount()));
#else
    m_usageLabel->hide();
#endif
    
    QString menuPathString = app->menuPath();
    if (!menuPathString.isEmpty()) {
        m_menuPathLabel->setText(menuPathString);
    } else {
        m_menuPathWidget->hide();
    }

    updateActionButton();

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
         m_version->setText(app->package()->latin1Name() % ' ' %
                            app->package()->availableVersion());
    } else {
         m_version->setText(app->package()->latin1Name() % ' ' %
                            app->package()->installedVersion());
    }

    m_license->setText(app->license());

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
    connect(reviewsBackend, SIGNAL(reviewsReady(Application*,QList<Review*>)),
            this, SLOT(populateReviews(Application*,QList<Review*>)));
    reviewsBackend->fetchReviews(app);

    fetchScreenshot(QApt::Thumbnail);
}

void ApplicationDetailsWidget::fetchScreenshot(QApt::ScreenshotType screenshotType)
{
    if (m_screenshotFile) {
        m_screenshotFile->deleteLater();
        m_screenshotFile = 0;
    }
    m_screenshotFile = new KTemporaryFile;
    m_screenshotFile->setPrefix("muon");
    m_screenshotFile->setSuffix(".png");
    m_screenshotFile->open();

    switch (screenshotType) {
    case QApt::Thumbnail: {
        QObject *object = m_screenshotView->rootObject();
        if (object) {
            object->setProperty("source", m_app->screenshotUrl(QApt::Thumbnail).pathOrUrl());
        }
        break;
    }
    case QApt::Screenshot: {
        KIO::FileCopyJob *getJob = KIO::file_copy(m_app->screenshotUrl(screenshotType),
                               m_screenshotFile->fileName(), -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(screenshotFetched(KJob*)));
        break;
    }
    case QApt::UnknownType:
    default:
        break;
    }
}

void ApplicationDetailsWidget::screenshotFetched(KJob *job)
{
    if (job->error()) {
        return;
    }

    m_fadeBlur->setDirection(QAbstractAnimation::Forward);
    m_fadeBlur->start();

    ScreenShotOverlay *overlay = new ScreenShotOverlay(m_screenshotFile->fileName(), viewport(), this);
    connect(overlay, SIGNAL(destroyed(QObject*)), this, SLOT(overlayClosed()));
}

void ApplicationDetailsWidget::overlayClosed()
{
    QObject *item = m_screenshotView->rootObject();
    connect(item, SIGNAL(thumbnailClicked()), this, SLOT(screenshotLabelClicked()));

    m_fadeBlur->setDirection(QAbstractAnimation::Backward);
    m_fadeBlur->start();

    unsetCursor();
}

void ApplicationDetailsWidget::screenshotLabelClicked()
{
    QObject *item = m_screenshotView->rootObject();
    disconnect(item, SIGNAL(thumbnailClicked()), this, SLOT(screenshotLabelClicked()));
    fetchScreenshot(QApt::Screenshot);
}

void ApplicationDetailsWidget::actionButtonClicked()
{
    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status Progress text when waiting", "Waiting"));

    // TODO: update packages
    if (m_app->package()->isInstalled()) {
        emit removeButtonClicked(m_app);
    } else {
        emit installButtonClicked(m_app);
    }
}

void ApplicationDetailsWidget::cancelButtonClicked()
{
    emit cancelButtonClicked(m_app);

    m_progressBar->hide();
    m_actionButton->show();
}

void ApplicationDetailsWidget::populateAddons()
{
    QApt::PackageList addons = m_app->addons();

    if (!addons.isEmpty()) {
        m_addonsWidget->setAddons(addons);
        m_addonsWidget->show();
    }
}

void ApplicationDetailsWidget::populateReviews(Application *app, const QList<Review *> &reviews)
{
    if (app != m_app) {
        return;
    }

    m_reviewsWidget->addReviews(reviews);
}

void ApplicationDetailsWidget::addonsApplyButtonClicked(const QHash<QApt::Package *,
                                                        QApt::Package::State> &changedAddons)
{
    emit installButtonClicked(m_app, changedAddons);

    m_actionButton->hide();
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat(i18nc("@info:status Progress text when waiting", "Waiting"));
}

void ApplicationDetailsWidget::applicationRunningChanged(bool running)
{
    m_actionButton->setVisible(!running);
    m_progressBar->setVisible(running);
}

void ApplicationDetailsWidget::applicationDownloadingChanged(bool downloading)
{
    m_cancelButton->setVisible(downloading);
}

void ApplicationDetailsWidget::progressChanged()
{
    m_progressBar->setValue(m_listener->progress());
}

void ApplicationDetailsWidget::progressCommentChanged()
{
    m_progressBar->setFormat(m_listener->comment());
}

void ApplicationDetailsWidget::updateActionButton()
{
    if (!m_app)
        return;

    if (!m_app->package()->isInstalled()) {
        m_statusLabel->setText(MuonStrings::global()->packageStateName(QApt::Package::NotInstalled));
        m_actionButton->setText(i18nc("@action", "Install"));
        m_actionButton->setIcon(KIcon("download"));
        m_actionButton->show();
    } else {
        m_statusLabel->setText(MuonStrings::global()->packageStateName(QApt::Package::Installed));
        m_actionButton->setText(i18nc("@action", "Remove"));
        m_actionButton->setIcon(KIcon("edit-delete"));
    }
}

#include "ApplicationDetailsWidget.moc"
