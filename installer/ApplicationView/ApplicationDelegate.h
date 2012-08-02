/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *   Copyright (C) 2008 Daniel Nicoletti <dantti85-pk@yahoo.com.br>
 *   Copyright (C) 2010 Jonathan Thomas <echidnaman@kubuntu.org>
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

#ifndef APPLICATIONDELEGATE_H
#define APPLICATIONDELEGATE_H

// KDE includes
#include <KExtendableItemDelegate>
#include <KIcon>

class AbstractResource;
class KIconLoader;
class KRatingPainter;

class AbstractResourcesBackend;
class ApplicationExtender;

/**
 * Delegate for displaying the applications
 */
class ApplicationDelegate: public KExtendableItemDelegate
{
    Q_OBJECT
public:
    ApplicationDelegate(QAbstractItemView *parent, AbstractResourcesBackend *backend);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index);
    void setShowInfoButton(bool show);

private:
    AbstractResourcesBackend *m_appBackend;

    QSize   m_buttonSize;
    QPersistentModelIndex m_oldIndex;
    ApplicationExtender *m_extender;
    KRatingPainter *m_ratingPainter;
    QPixmap m_emblem;
    bool m_showInfoButton;

    int calcItemHeight(const QStyleOptionViewItem &option) const;

public Q_SLOTS:
    void itemActivated(QModelIndex index);
    void invalidate();

Q_SIGNALS:
    void showExtendItem(const QModelIndex &index);
    void infoButtonClicked(AbstractResource *app);
    void installButtonClicked(AbstractResource *app);
    void removeButtonClicked(AbstractResource *app);
    void cancelButtonClicked(AbstractResource *app);
};

#endif
