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

#include "PackageDelegate.h"

// Qt
#include <QApplication>
#include <QtGui/QPainter>

// KDE
#include <KIcon>
#include <KIconLoader>

// Own
#include "../MuonStrings.h"
#include "PackageModel.h"

PackageDelegate::PackageDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
    , m_icon(KIcon("application-x-deb"))
    , m_supportedEmblem(KIcon("ubuntu-logo").pixmap(QSize(12,12)))
    , m_lockedEmblem(KIcon("object-locked").pixmap(QSize(12,12)))
{
    m_spacing  = 4;

    m_iconSize = KIconLoader::SizeSmallMedium;

    m_strings = new MuonStrings(this);
}

PackageDelegate::~PackageDelegate()
{
}

void PackageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    paintBackground(painter, option);

    switch (index.column()) {
    case 0:
        paintPackageName(painter, option, index);
        break;
    case 1: // Status
    case 2: // Action
        paintText(painter, option, index);
        break;
    default:
        break;
    }
}

void PackageDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option) const
{
    QStyleOptionViewItemV4 opt(option);
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
}

void PackageDelegate::paintPackageName(QPainter *painter, const QStyleOptionViewItem &option , const QModelIndex &index) const
{
    int left = option.rect.left();
    int top = option.rect.top();
    int width = option.rect.width();

    bool leftToRight = (painter->layoutDirection() == Qt::LeftToRight);

    QColor foregroundColor = (option.state.testFlag(QStyle::State_Selected)) ?
                             option.palette.color(QPalette::HighlightedText) : option.palette.color(QPalette::Text);

    // Pixmap that the text/icon goes in
    QPixmap pixmap(option.rect.size());
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.translate(-option.rect.topLeft());

    m_icon.paint(&p,
                 leftToRight ? left + m_spacing : left + width - m_spacing - m_iconSize,
                 top + m_spacing,
                 m_iconSize,
                 m_iconSize,
                 Qt::AlignCenter,
                 QIcon::Normal);

    int state = index.data(PackageModel::StatusRole).toInt();

    if (state & QApt::Package::IsPinned) {
        p.drawPixmap(left + m_iconSize - m_lockedEmblem.width()/2,
                     top + option.rect.height() - 1.5*m_lockedEmblem.height(),
                     m_lockedEmblem);
    } else if (index.data(PackageModel::SupportRole).toBool()) {
        p.drawPixmap(left + m_iconSize - m_lockedEmblem.width()/2,
                     top + option.rect.height() - 1.5*m_lockedEmblem.height(),
                     m_supportedEmblem);
    }

    // Text
    QStyleOptionViewItem name_item(option);
    QStyleOptionViewItem description_item(option);

    description_item.font.setPointSize(name_item.font.pointSize() - 1);

    int textInner = 2 * m_spacing + m_iconSize;
    const int itemHeight = calcItemHeight(option);

    p.setPen(foregroundColor);
    p.drawText(left + (leftToRight ? textInner : 0),
               top + 1,
               width - textInner,
               itemHeight / 2,
               Qt::AlignBottom | Qt::AlignLeft,
               index.data(PackageModel::NameRole).toString());

    p.drawText(left + (leftToRight ? textInner : 0) + 10,
               top + itemHeight / 2,
               width - textInner,
               itemHeight / 2,
               Qt::AlignTop | Qt::AlignLeft,
               index.data(PackageModel::DescriptionRole).toString());

    QLinearGradient gradient;

    // Gradient part of the background - fading of the text at the end
    if (leftToRight) {
        gradient = QLinearGradient(left + width - m_spacing - 16 /*fade length*/, 0,
                                   left + width - m_spacing, 0);
        gradient.setColorAt(0, Qt::white);
        gradient.setColorAt(1, Qt::transparent);
    } else {
        gradient = QLinearGradient(left + m_spacing, 0,
                                   left + m_spacing + 16, 0);
        gradient.setColorAt(0, Qt::transparent);
        gradient.setColorAt(1, Qt::white);
    }

    QRect paintRect = option.rect;
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.fillRect(paintRect, gradient);

    p.end();

    painter->drawPixmap(option.rect.topLeft(), pixmap);
}

void PackageDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option , const QModelIndex &index) const
{
    int state;
    QString text;
    QPen pen;

    switch (index.column()) {
    case 1:
        state = index.data(PackageModel::StatusRole).toInt();

        if (state & QApt::Package::NowBroken) {
            text = m_strings->packageStateName(QApt::Package::NowBroken);
            pen.setColor(Qt::red);
            break;
        }

        if (state & QApt::Package::Installed) {
            text = m_strings->packageStateName(QApt::Package::Installed);
            pen.setColor(Qt::darkGreen);

            if (state & QApt::Package::Upgradeable) {
                text = m_strings->packageStateName(QApt::Package::Upgradeable);
                pen.setColor(Qt::darkYellow);
            }
        } else {
            text = m_strings->packageStateName(QApt::Package::NotInstalled);
            pen.setColor(Qt::blue);
        }
        break;
    case 2:
        state = index.data(PackageModel::ActionRole).toInt();

        if (state & QApt::Package::ToKeep) {
            text = m_strings->packageStateName(QApt::Package::ToKeep);
            pen.setColor(Qt::blue);
            // No other "To" flag will be set if we are keeping
            break;
        }

        if (state & QApt::Package::ToInstall) {
            text = m_strings->packageStateName(QApt::Package::ToInstall);
            pen.setColor(Qt::darkGreen);
        }

        if (state & QApt::Package::ToUpgrade) {
            text = m_strings->packageStateName(QApt::Package::ToUpgrade);
            pen.setColor(Qt::darkYellow);
            break;
        }

        if (state & QApt::Package::ToRemove) {
            text = m_strings->packageStateName(QApt::Package::ToRemove);
            pen.setColor(Qt::red);
        }

        if (state & QApt::Package::ToPurge) {
            text = m_strings->packageStateName(QApt::Package::ToPurge);
            pen.setColor(Qt::red);
            break;
        }

        if (state & QApt::Package::ToReInstall) {
            text = m_strings->packageStateName(QApt::Package::ToReInstall);
            pen.setColor(Qt::darkGreen);
            break;
        }

        if (state & QApt::Package::ToDowngrade) {
            text = m_strings->packageStateName(QApt::Package::ToDowngrade);
            pen.setColor(Qt::darkYellow);
            break;
        }
        break;
    }

    QFont font = option.font;
    QFontMetrics fontMetrics(font);

    int x = option.rect.x() + m_spacing;
    int y = option.rect.y() + calcItemHeight(option) / 4 + fontMetrics.height() -1;
    int width = option.rect.width();

    painter->setPen(pen);
    painter->drawText(x, y, fontMetrics.elidedText(text, option.textElideMode, width));
}

QSize PackageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);

    QSize size;
    QFontMetrics metric = QFontMetrics(option.font);

    switch (index.column()) {
    case 0:
        size.setWidth(metric.width(index.data(PackageModel::DescriptionRole).toString()));
        break;
    case 1:
        size.setWidth(metric.width(index.data(PackageModel::StatusRole).toString()));
        break;
    case 2:
        size.setWidth(metric.width(index.data(PackageModel::ActionRole).toString()));
        break;
    default:
        break;
    }
    size.setHeight(option.fontMetrics.height() * 2 + m_spacing);

    return size;
}

int PackageDelegate::calcItemHeight(const QStyleOptionViewItem &option) const
{
    // Painting main column
    QStyleOptionViewItem name_item(option);
    QStyleOptionViewItem description_item(option);

    description_item.font.setPointSize(name_item.font.pointSize() - 1);

    int textHeight = QFontInfo(name_item.font).pixelSize() + QFontInfo(description_item.font).pixelSize();
    return qMax(textHeight, m_iconSize) + 2 * m_spacing;
}

#include "PackageDelegate.moc"
