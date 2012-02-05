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

#include "ReviewWidget.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>

#include <KGlobal>
#include <KLocale>
#include <Nepomuk/KRatingWidget>

#include <LibQApt/Package>

#include "Review.h"

ReviewWidget::ReviewWidget(QWidget *parent)
        : KVBox(parent)
{
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setMargin(0);
    headerWidget->setLayout(headerLayout);

    m_ratingWidget = new KRatingWidget(headerWidget);
    m_ratingWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_ratingWidget->setPixmapSize(16);
    m_summaryLabel = new QLabel(headerWidget);
    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_nameDateLabel = new QLabel(headerWidget);
    m_nameDateLabel->setEnabled(false);

    headerLayout->addWidget(m_ratingWidget);
    headerLayout->addWidget(m_summaryLabel);
    headerLayout->addWidget(headerSpacer);
    headerLayout->addWidget(m_nameDateLabel);

    m_reviewLabel = new QLabel(this);
    m_reviewLabel->setWordWrap(true);

    m_versionLabel = new QLabel(this);
    m_versionLabel->setEnabled(false);
    m_versionLabel->hide();

    QWidget *usefulnessWidget = new QWidget(this);
    QHBoxLayout *usefulnessLayout = new QHBoxLayout(usefulnessWidget);
    usefulnessLayout->setMargin(0);
    usefulnessWidget->setLayout(usefulnessLayout);

    m_usefulnessLabel = new QLabel(usefulnessWidget);
    // TODO: Report usefulness/inappropriateness once submitting works

    usefulnessLayout->addWidget(m_usefulnessLabel);
}

ReviewWidget::~ReviewWidget()
{
}

void ReviewWidget::setReview(Review *review)
{
    if (!review->package())
        return;

    m_ratingWidget->setRating(review->rating());

    m_summaryLabel->setText(QLatin1Literal("<b>") % review->summary()
                            % QLatin1Literal("</b>"));

    QString date = KGlobal::locale()->formatDate(review->creationDate().date(), KLocale::ShortDate);
    m_nameDateLabel->setText(i18nc("@label Formatted: username, date",
                                   "%1, %2",
                                   review->reviewer(), date));

    m_reviewLabel->setText(review->reviewText());

    const QString reviewUpstream = QApt::Package::upstreamVersion(review->packageVersion());
    const QString currentUpstream = review->package()->upstreamVersion();

    int res = QApt::Package::compareVersion(reviewUpstream, currentUpstream);

    if (res < 0) {
        m_versionLabel->setText(QLatin1Literal("<i>") %
                                i18nc("@label", "This review was written for an older version "
                                                "(Version: %1)", review->packageVersion()) %
                                QLatin1Literal("</i>"));
        m_versionLabel->show();
    }

    if (review->usefulnessTotal()) {
        m_usefulnessLabel->setText(i18ncp("@label", "%1 out of %2 person found this review useful",
                                          "%1 out of %2 people found this review useful",
                                          review->usefulnessFavorable(), review->usefulnessTotal()));
    }
}

#include "ReviewWidget.moc"
