/***************************************************************************
 *   Copyright © 2010 Guillaume Martres <smarter@ubuntu.com>               *
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

#include "DownloadDelegate.h"

// Qt includes
#include <QtGui/QPainter>

// KDE includes
#include <KApplication>
#include <KLocale>

// LibQApt includes
#include <LibQApt/Globals>

// Own includes
#include "DownloadModel.h"

DownloadDelegate::DownloadDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_spacing(4)
{
}

DownloadDelegate::~DownloadDelegate()
{
}

void DownloadDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    switch (index.column()) {
        case 0: {
            QFont font = option.font;
            QFontMetrics fontMetrics(font);

            int x = option.rect.x();
            int y = option.rect.y() + calcItemHeight(option);
            int width = option.rect.width();

            QPen pen;
            painter->setPen(pen);
            QString text = index.data(DownloadModel::NameRole).toString();
            painter->drawText(x, y, fontMetrics.elidedText(text, option.textElideMode, width));
            break;
        }
        case 1: {
            QFont font = option.font;
            QFontMetrics fontMetrics(font);

            int x = option.rect.x();
            int y = option.rect.y() + calcItemHeight(option);
            int width = option.rect.width();

            QPen pen;
            painter->setPen(pen);
            QString text = index.data(DownloadModel::URIRole).toString();
            painter->drawText(x, y, fontMetrics.elidedText(text, option.textElideMode, width));
            break;
        }
        case 2: {
            int percentage = index.data(DownloadModel::PercentRole).toInt();
            int status = index.data(DownloadModel::StatusRole).toInt();
            QString text;

            switch (status) {
                case QApt::DownloadFetch:
                    break;
                case QApt::HitFetch:
                    text = i18nc("@info:status", "Done");
                    break;
                case QApt::IgnoredFetch:
                    text = i18nc("@info:status", "Ignored");
                    break;
                default:
                    text = QString::number(percentage) + '%';
                    break;
            }

            if (percentage == 100) {
                text = i18nc("@info:status", "Done");
            }

            QStyleOptionProgressBar progressBarOption;
            progressBarOption.rect = option.rect;
            progressBarOption.minimum = 0;
            progressBarOption.maximum = 100;
            progressBarOption.progress = percentage;
            progressBarOption.text = text;
            progressBarOption.textVisible = true;

            KApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
            break;
        }
    }
}

int DownloadDelegate::calcItemHeight(const QStyleOptionViewItem &option) const
{
    // Painting main column
    QStyleOptionViewItem name_item(option);

    int textHeight = QFontInfo(name_item.font).pixelSize();
    return textHeight;
}

#include "DownloadDelegate.moc"
