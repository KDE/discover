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

class KIconLoader;
class KRatingPainter;

class Application;
class ApplicationBackend;
class ApplicationExtender;

/**
 * Delegate for displaying the applications
 */
class ApplicationDelegate: public KExtendableItemDelegate
{
    Q_OBJECT
public:
    ApplicationDelegate(QAbstractItemView *parent, ApplicationBackend *backend);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index);

private:
    ApplicationBackend *m_appBackend;
    int calcItemHeight(const QStyleOptionViewItem &option) const;

    KIcon   m_installIcon;
    QString m_installString;
    KIcon   m_removeIcon;
    QString m_removeString;
    QSize   m_buttonSize;
    QSize   m_buttonIconSize;

    ApplicationExtender *m_extender;
    mutable QPersistentModelIndex m_oldIndex;

    KRatingPainter *m_ratingPainter;

public Q_SLOTS:
    void itemActivated(QModelIndex index);
    void invalidate();

Q_SIGNALS:
    void showExtendItem(const QModelIndex &index);
    void infoButtonClicked(Application *app);
    void installButtonClicked(Application *app);
    void removeButtonClicked(Application *app);
    void cancelButtonClicked(Application *app);
};

#endif
