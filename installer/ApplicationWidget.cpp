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

#include "ApplicationWidget.h"

#include <QApplication>
#include <QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

#include <KGlobal>
#include <KIcon>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KTemporaryFile>
#include <KDebug>

#include <LibQApt/Package>

#include "Application.h"
#include "ClickableLabel.h"
#include "effects/GraphicsOpacityDropShadowEffect.h"

#define BLUR_RADIUS 15

ApplicationWidget::ApplicationWidget(QWidget *parent, Application *app)
    : QScrollArea(parent)
    , m_app(app)
    , m_screenshotFile(0)
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
    // FIXME: Always keep label size at 48x48, and render the largest size
    // we can up to that point. Otherwise some icons will be blurry
    m_iconLabel->setPixmap(KIcon(app->icon()).pixmap(48,48));

    QWidget *nameDescWidget = new QWidget(headerWidget);
    QVBoxLayout *nameDescLayout = new QVBoxLayout(nameDescWidget);
    m_nameLabel = new QLabel(nameDescWidget);
    m_nameLabel->setText(QLatin1Literal("<h1>") % app->name() % QLatin1Literal("</h1>"));
    m_nameLabel->setAlignment(Qt::AlignLeft);
    m_shortDescLabel = new QLabel(nameDescWidget);
    m_shortDescLabel->setText(app->comment());

    nameDescLayout->addWidget(m_nameLabel);
    nameDescLayout->addWidget(m_shortDescLabel);

    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(nameDescWidget);
    headerLayout->addWidget(headerSpacer);

    // Long description and Screenshot
    QWidget *body = new QWidget(widget);
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);

    m_longDescLabel = new QLabel(body);
    m_longDescLabel->setWordWrap(true);
    m_longDescLabel->setText(app->package()->longDescription());
    m_longDescLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_screenshotLabel = new ClickableLabel(body);
    m_screenshotLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_screenshotLabel->setMinimumSize(170, 130);

    GraphicsOpacityDropShadowEffect *shadow = new GraphicsOpacityDropShadowEffect(m_screenshotLabel);
    shadow->setBlurRadius(BLUR_RADIUS);
    shadow->setOpacity(0);
    shadow->setOffset(2);
    shadow->setColor(QApplication::palette().dark().color());
    m_screenshotLabel->setGraphicsEffect(shadow);
    fetchScreenshot(QApt::Thumbnail);
    connect(m_screenshotLabel, SIGNAL(clicked()), this, SLOT(screenshotLabelClicked()));

    m_fadeScreenshot = new QPropertyAnimation(shadow, "opacity");
    m_fadeScreenshot->setDuration(500);
    m_fadeScreenshot->setStartValue(qreal(0));
    m_fadeScreenshot->setEndValue(qreal(1));

    bodyLayout->addWidget(m_longDescLabel);
    bodyLayout->addWidget(m_screenshotLabel);

    m_websiteLabel = new QLabel(this);

    QString homepageUrl = app->package()->homepage();
    if (!homepageUrl.isEmpty()) {
        QString websiteString = i18nc("@label visible text for an app's URL", "Website");
        m_websiteLabel->setAlignment(Qt::AlignLeft);
        m_websiteLabel->setText(QLatin1Literal("<a href=\"") % homepageUrl % "\">" %
                                websiteString % QLatin1Literal("</a>"));
        m_websiteLabel->setToolTip(homepageUrl);
        m_websiteLabel->setOpenExternalLinks(true);
    } else {
        m_websiteLabel->hide();
    }

    // Technical details
    QWidget *detailsWidget = new QWidget(widget);
    QGridLayout *detailsGrid = new QGridLayout(detailsWidget);

    // detailsGrid, row 0
    QLabel *sizeLabel = new QLabel(detailsWidget);
    sizeLabel->setText(i18nc("@label Label preceeding the app size", "Total Size:"));
    m_size = new QLabel(detailsWidget);
    if (!app->package()->isInstalled()) {
        m_size->setText(i18nc("@info app size", "%1 to download, %2 on disk",
                              KGlobal::locale()->formatByteSize(app->package()->downloadSize()),
                              KGlobal::locale()->formatByteSize(app->package()->availableInstalledSize())));
    } else {
        m_size->setText(i18nc("@info app size", "%1 on disk",
                              KGlobal::locale()->formatByteSize(app->package()->currentInstalledSize())));
    }
    detailsGrid->addWidget(sizeLabel, 0, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_size, 0, 1, Qt::AlignLeft);

    // detailsGrid, row 1
    QLabel *versionLabel = new QLabel(detailsWidget);
    versionLabel->setText(i18nc("@label Label preceeding the app version", "Version:"));
    m_version = new QLabel(detailsWidget);
    if (!app->package()->isInstalled()) {
         m_version->setText(app->package()->availableVersion());
    } else {
         m_version->setText(app->package()->installedVersion());
    }
    detailsGrid->addWidget(versionLabel, 1, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_version, 1, 1, Qt::AlignLeft);

    // detailsGrid, row 2
    QLabel *licenseLabel = new QLabel(detailsWidget);
    licenseLabel->setText(i18nc("@label Label preceeding the app license", "Liscense:"));
    m_license = new QLabel(detailsWidget);
    if (app->package()->component() == "main" ||
        app->package()->component() == "universe") {
        m_license->setText(i18nc("@info license", "Open Source"));
    } else if (app->package()->component() == "restricted") {
        m_license->setText(i18nc("@info license", "Proprietary"));
    }
    detailsGrid->addWidget(licenseLabel, 2, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_license, 2, 1, Qt::AlignLeft);

    // detailsGrid, row 3
    QLabel *supportLabel = new QLabel(detailsWidget);
    supportLabel->setText(i18nc("@label Label preceeding the app support", "Support:"));
    m_support = new QLabel(detailsWidget);
    if (app->package()->isSupported()) {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical provides critical updates for %1 until %2",
                                 app->name(), app->package()->supportedUntil()));
    } else {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical does not provide updates for %1. Some updates "
                                 "may be provided by the Ubuntu community", app->name()));
    }

    detailsGrid->addWidget(supportLabel, 3, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_support, 3, 1, Qt::AlignLeft);

    detailsGrid->setColumnStretch(1,1);

    QWidget *verticalSpacer = new QWidget(this);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);


    layout->addWidget(headerWidget);
    layout->addWidget(body);
    layout->addWidget(m_websiteLabel);
    layout->addWidget(detailsWidget);
    layout->addWidget(verticalSpacer);

    setWidget(widget);
}

ApplicationWidget::~ApplicationWidget()
{
}

void ApplicationWidget::fadeInDetails()
{
    m_fadeDetails->setDirection(QAbstractAnimation::Forward);
    m_fadeDetails->start();
}

void ApplicationWidget::fadeInScreenshot()
{
    m_fadeScreenshot->setDirection(QAbstractAnimation::Forward);
    m_fadeScreenshot->start();
}

void ApplicationWidget::fetchScreenshot(QApt::ScreenshotType screenshotType)
{
    if (m_screenshotFile) {
        m_screenshotFile->deleteLater();
        m_screenshotFile = 0;
    }
    m_screenshotFile = new KTemporaryFile;
    m_screenshotFile->setPrefix("muon");
    m_screenshotFile->setSuffix(".png");
    m_screenshotFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_app->package()->screenshotUrl(screenshotType),
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

void ApplicationWidget::thumbnailFetched(KJob *job)
{
    if (job->error()) {
        m_screenshotLabel->hide();
        return;
    }
    m_screenshotLabel->show();
    m_screenshotLabel->setPixmap(QPixmap(m_screenshotFile->fileName()).scaled(160,120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_screenshotLabel->setCursor(Qt::PointingHandCursor);
    fadeInScreenshot();
}

void ApplicationWidget::screenshotFetched(KJob *job)
{
    if (job->error()) {
        return;
    }

    // Create KDialog for showing screenie
    kDebug() << "big screenie fetched";
}

void ApplicationWidget::screenshotLabelClicked()
{
    fetchScreenshot(QApt::Screenshot);
}

#include "ApplicationWidget.moc"


