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

#include <KApplication>

#include <QPainter>

#include "../PackageModel/PackageModel.h"

DownloadDelegate::DownloadDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}


QSize DownloadDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size;

    size.setWidth(option.fontMetrics.height() * 4);
    size.setHeight(option.fontMetrics.height() * 2);
    return size;
}

void DownloadDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //initStyleOption(option, index);

    if (index.column() == 0) {
        QFont font = option.font;
        QFontMetrics fontMetrics(font);

        int x = option.rect.x();
        int y = option.rect.y() + fontMetrics.height();
        int width = option.rect.width();

        QPen pen;
        painter->setPen(pen);
        QString text = index.data(PackageModel::NameRole).toString();
        painter->drawText(x, y, fontMetrics.elidedText(text, option.textElideMode, width));
        //QStyledItemDelegate::paint(painter, option, index);
    } else {
        int percentage = index.data(PackageModel::StatusRole).toInt();

        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = percentage;
        progressBarOption.text = QString::number(percentage) + "%";
        progressBarOption.textVisible = true;

        KApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
    }
}

#include "DownloadDelegate.moc"
