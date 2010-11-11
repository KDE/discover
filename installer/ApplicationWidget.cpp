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

#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

#include <KGlobal>
#include <KIcon>
#include <KLocale>

#include <LibQApt/Package>

#include "Application.h"
#include "ClickableLabel.h"

ApplicationWidget::ApplicationWidget(QWidget *parent, Application *app)
    : QScrollArea(parent)
    , m_app(app)
{
    setFrameShape(QFrame::NoFrame);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    widget->setLayout(layout);

    // Header
    QWidget *headerWidget = new QWidget(widget);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerWidget->setLayout(headerLayout);

    m_iconLabel = new QLabel(headerWidget);
    // FIXME: Bigger if possible
    m_iconLabel->setPixmap(KIcon(app->icon()).pixmap(32,32));

    QWidget *nameDescWidget = new QWidget(headerWidget);
    QVBoxLayout *nameDescLayout = new QVBoxLayout(nameDescWidget);
    m_nameLabel = new QLabel(nameDescWidget);
    m_nameLabel->setText(QLatin1Literal("<h1>") % app->name() % QLatin1Literal("</h1>"));
    m_nameLabel->setAlignment(Qt::AlignLeft);
    m_shortDescLabel = new QLabel(nameDescWidget);
    m_shortDescLabel->setText(app->comment());
    m_shortDescLabel->setAlignment(Qt::AlignLeft);

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
    m_screenshotLabel = new ClickableLabel(body);
    m_screenshotLabel->setPixmap(KIcon(app->icon()).pixmap(48,48)); // FIXME: Use screenshot

    bodyLayout->addWidget(m_longDescLabel);
    bodyLayout->addWidget(m_screenshotLabel);

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
    m_support->setWordWrap(true);
    if (app->package()->isSupported()) {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical provides critical updates for %1 until %2",
                                 app->name(), app->package()->supportedUntil()));
    } else {
        m_support->setText(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                 "Canonical does not provide updates for %1. Some updates "
                                 "may be provided by the Ubuntu community", app->name()));
    }

    // Spacer
    QWidget *verticalSpacer = new QWidget(widget);
    verticalSpacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    detailsGrid->addWidget(supportLabel, 3, 0, Qt::AlignRight);
    detailsGrid->addWidget(m_support, 3, 1, Qt::AlignLeft);
    detailsGrid->setColumnStretch(1, 1);


    layout->addWidget(headerWidget);
    layout->addWidget(body);
    layout->addWidget(detailsWidget);
    layout->addWidget(verticalSpacer);

    setWidget(widget);
}

ApplicationWidget::~ApplicationWidget()
{
}

#include "ApplicationWidget.moc"


