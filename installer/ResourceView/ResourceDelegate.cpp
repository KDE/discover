/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *   Copyright (C) 2008 Daniel Nicoletti <dantti85-pk@yahoo.com.br>
 *   Copyright (C) 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ResourceDelegate.h"

// Qt includes
#include <QApplication>
#include <QAbstractItemView>
#include <QPainter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeView>

// KDE includes
#include <KApplication>
#include <KIconLoader>
#include <KGlobal>
#include <KLocalizedString>
#include <KStandardDirs>
#include <Nepomuk/KRatingPainter>
#include <KDebug>

// Libmuon includes
#include <Transaction/TransactionModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>

// Own includes
#include "ResourceExtender.h"

#define FAV_ICON_SIZE 24
#define EMBLEM_ICON_SIZE 8
#define UNIVERSAL_PADDING 4
#define MAIN_ICON_SIZE 32

ResourceDelegate::ResourceDelegate(QAbstractItemView *parent)
  : KExtendableItemDelegate(parent),
    m_extender(0),
    m_showInfoButton(true)
{
    // To get sizing.
    QPushButton button, button2;
    QIcon icon(QIcon::fromTheme("edit-delete"));

    button.setText(i18n("Install"));
    button.setIcon(icon);
    button2.setText(i18n("Remove"));
    button2.setIcon(icon);
    m_buttonSize = button.sizeHint();
    int width = qMax(button.sizeHint().width(), button2.sizeHint().width());
    width = qMax(width, button2.sizeHint().width());
    m_buttonSize.setWidth(width);

    m_emblem = QIcon::fromTheme("dialog-ok").pixmap(QSize(16, 16));
    m_ratingPainter = new KRatingPainter;
}

void ResourceDelegate::paint(QPainter *painter,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    bool leftToRight = (painter->layoutDirection() == Qt::LeftToRight);

    QStyleOptionViewItemV4 opt(option);
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    painter->save();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    painter->restore();

    // paint the extender
    KExtendableItemDelegate::paint(painter, opt, index);

    int leftCount;
    if (leftToRight) {
        opt.rect.setLeft(option.rect.left() + UNIVERSAL_PADDING);
        leftCount = opt.rect.left() + UNIVERSAL_PADDING;
    } else {
        opt.rect.setRight(option.rect.right() - UNIVERSAL_PADDING);
        leftCount = opt.rect.width() - (UNIVERSAL_PADDING + MAIN_ICON_SIZE);
    }

    int left = opt.rect.left();
    int top = opt.rect.top();
    int width = opt.rect.width();

    QRect rect = opt.rect;

    if (leftToRight) {
        rect.setLeft(left + width - (m_buttonSize.width() + UNIVERSAL_PADDING));
        width -= m_buttonSize.width() + UNIVERSAL_PADDING;
    } else {
        rect.setLeft(left + UNIVERSAL_PADDING);
        left += m_buttonSize.width() + UNIVERSAL_PADDING;
    }
    // Calculate the top of the ratings widget which is the item height - the widget height size divided by 2
    // this give us a little value which is the top and bottom margin
    rect.setTop(rect.top() + ((calcItemHeight(option) - m_buttonSize.height()) / 2));
    rect.setSize(m_buttonSize); // the width and height sizes of the button

    bool transactionActive = index.data(ResourcesModel::ActiveRole).toBool();

    if (!transactionActive) {
        int rating = index.data(ResourcesModel::RatingRole).toInt();
        if (rating != -1) {
            m_ratingPainter->paint(painter, rect, rating);
        }
    } else {
        TransactionModel *transModel = TransactionModel::global();
        AbstractResource* res = qobject_cast<AbstractResource*>( index.data(ResourcesModel::ApplicationRole).value<QObject*>());
        Transaction *trans = transModel->transactionFromResource(res);
        QModelIndex transIndex = transModel->indexOf(trans);
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = transIndex.data(TransactionModel::ProgressRole).toInt();
        progressBarOption.text = transIndex.data(TransactionModel::StatusTextRole).toString();
        progressBarOption.textVisible = true;
        KApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
    }


    // selects the mode to paint the icon based on the info field
    QIcon::Mode iconMode = QIcon::Normal;
    if (option.state & QStyle::State_MouseOver) {
        iconMode = QIcon::Active;
    }

    QColor foregroundColor = (option.state.testFlag(QStyle::State_Selected))?
    option.palette.color(QPalette::HighlightedText):option.palette.color(QPalette::Text);

    // Painting main column
    QStyleOptionViewItem local_option_title(option);
    QStyleOptionViewItem local_option_normal(option);

    local_option_normal.font.setPointSize(local_option_normal.font.pointSize() - 1);

    QPixmap pixmap(option.rect.size());
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.translate(-option.rect.topLeft());

    // Main icon
    QIcon icon = QIcon::fromTheme(index.data(ResourcesModel::IconRole).toString());

    int iconSize = calcItemHeight(option) - 2 * UNIVERSAL_PADDING;
    icon.paint(&p,
               leftCount,
               top + UNIVERSAL_PADDING,
               iconSize,
               iconSize,
               Qt::AlignCenter,
               iconMode);

    if (index.data(ResourcesModel::InstalledRole).toBool()) {
        p.drawPixmap(leftCount, top + rect.height() - m_emblem.height()/2, m_emblem);
    }

    int textWidth;
    if (leftToRight) {
        // add the main icon
        leftCount += iconSize + UNIVERSAL_PADDING;
        textWidth = width - (leftCount - left);
    } else {
        leftCount -= UNIVERSAL_PADDING;
        textWidth = leftCount - left;
        leftCount = left;
    }

    // Text
    const int itemHeight = calcItemHeight(option);

    p.setPen(foregroundColor);
    // draw the top line
    int topTextHeight = QFontInfo(local_option_title.font).pixelSize();
    p.setFont(local_option_title.font);
    p.drawText(leftCount,
               top,
               textWidth,
               topTextHeight + UNIVERSAL_PADDING,
               Qt::AlignVCenter | Qt::AlignLeft,
               index.data(ResourcesModel::NameRole).toString());

    // draw the bottom line
    iconSize = topTextHeight + UNIVERSAL_PADDING;

    // store the original opacity
    qreal opa = p.opacity();
    if (!(option.state & QStyle::State_MouseOver) && !(option.state & QStyle::State_Selected)) {
        p.setOpacity(opa / 2.5);
    }

    p.setFont(local_option_normal.font);
    p.drawText(leftToRight ? leftCount + 0.5*iconSize: left - UNIVERSAL_PADDING,
               top + itemHeight / 2,
               textWidth - iconSize,
               QFontInfo(local_option_normal.font).pixelSize() + UNIVERSAL_PADDING,
               Qt::AlignTop | Qt::AlignLeft,
               index.data(ResourcesModel::CommentRole).toString());
    p.setOpacity(opa);

    painter->drawPixmap(option.rect.topLeft(), pixmap);
}

int ResourceDelegate::calcItemHeight(const QStyleOptionViewItem &option) const
{
    // Painting main column
    QStyleOptionViewItem local_option_title(option);
    QStyleOptionViewItem local_option_normal(option);

    local_option_normal.font.setPointSize(local_option_normal.font.pointSize() - 1);

    int textHeight = QFontInfo(local_option_title.font).pixelSize() + QFontInfo(local_option_normal.font).pixelSize();
    return textHeight + 3 * UNIVERSAL_PADDING;
}

bool ResourceDelegate::editorEvent(QEvent *event,
                                    QAbstractItemModel *model,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
{
    Q_UNUSED(option)
    if (event->type() == QEvent::MouseButtonRelease) {
       itemActivated(index);
    }

    return KExtendableItemDelegate::editorEvent(event, model, option, index);
}

QSize ResourceDelegate::sizeHint(const QStyleOptionViewItem &option,
        const QModelIndex &index ) const
{
    int width = (index.column() == 0) ? index.data(Qt::SizeHintRole).toSize().width() : FAV_ICON_SIZE + 2 * UNIVERSAL_PADDING;
    QSize ret(KExtendableItemDelegate::sizeHint(option, index));
    // remove the default size of the index
    ret -= QStyledItemDelegate::sizeHint(option, index);

    ret.rheight() += calcItemHeight(option);
    ret.rwidth()  += width;

    return ret;
}

void ResourceDelegate::itemActivated(QModelIndex index)
{
    if ((index == m_oldIndex && isExtended(index))) {
        return;
    }

    if (isExtended(m_oldIndex)) {
        disconnect(m_extender, SIGNAL(infoButtonClicked(AbstractResource*)),
                   this, SIGNAL(infoButtonClicked(AbstractResource*)));
        contractItem(m_oldIndex);

        m_extender->deleteLater();
        m_extender = 0;
    }

    QVariant appVarient = static_cast<const QAbstractItemModel*>(index.model())->data(index, ResourcesModel::ApplicationRole);
    AbstractResource *app = qobject_cast<AbstractResource*>(appVarient.value<QObject*>());

    QTreeView *view = static_cast<QTreeView*>(parent());
    m_extender = new ResourceExtender(view, app);
    m_extender->setShowInfoButton(m_showInfoButton);
    connect(m_extender, SIGNAL(infoButtonClicked(AbstractResource*)),
            this, SIGNAL(infoButtonClicked(AbstractResource*)));

    extendItem(m_extender, index);
    m_oldIndex = index;
}

void ResourceDelegate::invalidate()
{
    // If only contractAll was a Q_SLOT...
    contractAll();
}

void ResourceDelegate::setShowInfoButton(bool show)
{
    m_showInfoButton = show;
}
