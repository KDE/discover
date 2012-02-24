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

#include "ReviewsWidget.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

#include <KDialog>
#include <KLocale>

#include "Review.h"
#include "ReviewWidget.h"

bool reviewsGreaterThan(Review *lhs, Review *rhs)
{
    return *lhs > *rhs;
}

ReviewsWidget::ReviewsWidget(QWidget *parent)
        : KVBox(parent)
{
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setMargin(0);
    headerWidget->setLayout(headerLayout);

    m_expandButton = new QToolButton(headerWidget);
    m_expandButton->setAutoRaise(true);
    m_expandButton->setArrowType(Qt::DownArrow);
    connect(m_expandButton, SIGNAL(clicked()), this, SLOT(expandButtonClicked()));

    QLabel *titleLabel = new QLabel(headerWidget);
    titleLabel->setText(QLatin1Literal("<h3>") %
                        i18nc("@title", "Reviews") % QLatin1Literal("</h3>"));

    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    headerLayout->addWidget(m_expandButton);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(headerSpacer);

    m_reviewContainer = new QWidget(this);
    m_reviewLayout = new QVBoxLayout(m_reviewContainer);
    m_reviewLayout->setSpacing(2*KDialog::spacingHint());
    m_reviewContainer->setLayout(m_reviewLayout);

    m_statusLabel = new QLabel(m_reviewContainer);
    m_statusLabel->setAlignment(Qt::AlignHCenter);
    m_statusLabel->setText(i18nc("@info:status", "Loading reviews"));

    m_reviewLayout->addWidget(m_statusLabel);
}

ReviewsWidget::~ReviewsWidget()
{
}

void ReviewsWidget::expandButtonClicked()
{
    if (m_reviewContainer->isHidden()) {
        m_expandButton->setArrowType(Qt::DownArrow);
        m_reviewContainer->show();
    } else {
        m_reviewContainer->hide();
        m_expandButton->setArrowType(Qt::RightArrow);
    }
}

void ReviewsWidget::addReviews(QList<Review *> reviews)
{
    if (reviews.isEmpty()) {
        m_statusLabel->setText(i18nc("@info:status", "No reviews available"));
        return;
    } else {
        m_statusLabel->hide();
    }

    qSort(reviews.begin(), reviews.end(), reviewsGreaterThan);

    foreach (Review *review, reviews) {
        if (!review->shouldShow()) {
            continue;
        }
        ReviewWidget *reviewWidget = new ReviewWidget(m_reviewContainer);
        reviewWidget->setReview(review);

        m_reviewLayout->addWidget(reviewWidget);
    }
}

#include "ReviewsWidget.moc"
