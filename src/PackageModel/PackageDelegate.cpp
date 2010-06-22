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
#include <QAbstractItemView>
#include <QApplication>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>

// KDE
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KDebug>

// Own
#include "PackageModel.h"

PackageDelegate::PackageDelegate(QObject * parent)
        : QAbstractItemDelegate(parent)
{
    m_spacing    = 4;
    m_iconSize = KIconLoader::global()->currentSize(KIconLoader::Toolbar);
}

PackageDelegate::~PackageDelegate()
{
}

void PackageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
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
        kDebug() << "unexpected column";
    }
}

void PackageDelegate::paintBackground(QPainter* painter, const QStyleOptionViewItem& option) const
{
    QStyleOptionViewItemV4 opt(option);
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
}

void PackageDelegate::paintPackageName(QPainter* painter, const QStyleOptionViewItem& option , const QModelIndex &index) const
{
    int left = option.rect.left();
    int top = option.rect.top();
    int width = option.rect.width();

    bool leftToRight = (painter->layoutDirection() == Qt::LeftToRight);

    QColor foregroundColor = (option.state.testFlag(QStyle::State_Selected))?
        option.palette.color(QPalette::HighlightedText):option.palette.color(QPalette::Text);

    // Pixmap that the text/icon goes in
    QPixmap pixmap(option.rect.size());
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.translate(-option.rect.topLeft());

    KIcon *icon = new KIcon("application-x-deb");
    icon->paint(&p,
           leftToRight ? left + m_spacing : left + width - m_spacing - m_iconSize,
           top + m_spacing,
           m_iconSize,
           m_iconSize,
           Qt::AlignCenter,
           QIcon::Normal);

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
    delete icon;
}

void PackageDelegate::paintText(QPainter* painter, const QStyleOptionViewItem& option , const QModelIndex &index) const
{
    int state;
    QString text;
    QPen pen;

    switch (index.column()) {
        case 1:
            state = index.data(PackageModel::StatusRole).toInt();
            if (state & QApt::Package::Installed) {
                text = i18n("Installed");
                pen.setColor(Qt::darkGreen);

                if (state & QApt::Package::Upgradeable) {
                    text = i18n("Upgradeable");
                    pen.setColor(Qt::darkYellow);
                }
            } else {
                text = i18n("Not installed");
                pen.setColor(Qt::blue);
            }
            break;
        case 2:
            state = index.data(PackageModel::ActionRole).toInt();

            if (state & QApt::Package::NowBroken){
                text = i18n("Broken");
                pen.setColor(Qt::red);
            }

            if (state & QApt::Package::ToKeep) {
                text = i18n("No change");
                pen.setColor(Qt::blue);
            }

            if (state & QApt::Package::ToInstall) {
                text = i18n("Install");
                pen.setColor(Qt::darkGreen);
            }

            if (state & QApt::Package::ToReInstall) {
                text = i18n("Reinstall");
                pen.setColor(Qt::darkGreen);
            }

            if (state & QApt::Package::ToUpgrade) {
                text = i18n("Upgrade");
                pen.setColor(Qt::darkYellow);
            }

            if (state & QApt::Package::ToDowngrade) {
                text = i18n("Downgrade");
                pen.setColor(Qt::darkYellow);
            }

            if (state & QApt::Package::ToRemove) {
                text = i18n("Remove");
                pen.setColor(Qt::red);
            }

            if (state & QApt::Package::ToPurge) {
                text = i18n("Purge");
                pen.setColor(Qt::red);
            }
            break;
    }

    int x = option.rect.x() + m_spacing;
    int y = option.rect.y() + calcItemHeight(option)/2 + m_spacing;
    int width = option.rect.width();

    QFont font = option.font;
    QFontMetrics fontMetrics(font);

    painter->setPen(pen);
    painter->drawText(x, y, fontMetrics.elidedText(text, option.textElideMode, width));
}

QSize PackageDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    QSize size;

    // Doing this for realz would result in 90,000 iterations grabbing font metrics...
    // I fear for how columns will look by default for translations... FIXME if we can...
    size.setWidth(option.fontMetrics.height() * 4);
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
