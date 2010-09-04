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

#include "ApplicationModel.h"

#include <KIcon>
#include <KDebug>
#include <KLocale>
#include <KExtendableItemDelegate>

#include <libqapt/package.h>

#include <math.h>

ApplicationModel::ApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_apps()
    , m_maxPopcon(0)
{
}

ApplicationModel::~ApplicationModel()
{
}

int ApplicationModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_apps.count();
}

int ApplicationModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return false;
    }
    switch (role) {
        case NameRole:
            return m_apps.at(index.row())->name();
        case IconRole:
            return m_apps.at(index.row())->icon();
        case CommentRole:
            return m_apps.at(index.row())->comment();
        case StatusRole:
        case ActionRole:
            return m_apps.at(index.row())->package()->state();
        case PopconRole:
            // Take the log of the popcon score, divide by max +1, then multiply by number
            // of star steps. (10 in the case of KRatingsPainter
            return (int)(10* log(m_apps.at(index.row())->popconScore())/log(m_maxPopcon+1));
        case Qt::ToolTipRole:
            return QVariant();
    }

    return QVariant();
}

void ApplicationModel::setApplications(const QList<Application*> &list)
{
    // Remove check when sid has >= 4.7
#if QT_VERSION >= 0x040700
    m_apps.reserve(list.size());
#endif
    beginInsertRows(QModelIndex(), m_apps.count(), m_apps.count());
    m_apps = list;
    endInsertRows();
}

void ApplicationModel::setMaxPopcon(int popconScore)
{
    m_maxPopcon = popconScore;
}

void ApplicationModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_apps.size());
    m_apps.clear();
    m_maxPopcon = 0;
    endRemoveRows();
}

Application *ApplicationModel::applicationAt(const QModelIndex &index) const
{
    return m_apps.at(index.row());
}

QList<Application*> ApplicationModel::applications() const
{
    return m_apps;
}

#include "ApplicationModel.moc"
